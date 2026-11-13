// duke-display.c -- c4's display driver for the Build engine, in place of
// Engine/src/display.c (Ryan C. Gordon's SDL driver), which it follows
// function by function. Every build has this file: c4lc's for C4IX, C4DOS
// and the bare machine, and gcc's for the host. What differs between them
// is underneath, in the platform (duke_plat_*): libjs's display
// (duke-gui.c), or nothing at all (duke-host.c).
//
// The frame is an 8-bit buffer of xdim x ydim that the engine draws in,
// sent whole with its palette at each page flip. Any size in the mode list
// below can be chosen in the game (Options > Video Setup), and the
// display scales it to fill the screen. Keys arrive as a browser's key
// codes and become the PC scan codes Build reads; the mouse moves by its
// motion, relative, as SDL's grabbed mouse did.
//
// The clock is the machine's, in milliseconds, but in a test (-sum) it is
// one that moves only as the game runs (duke-sum.c), so that every build
// sees the same times and draws the same frames.
//
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman; see
// BUILDLIC.TXT. This file is not part of Ken Silverman's original release.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "build.h"
#include "display.h"

// the one definition of each (display.h); nothing here sets them, as the
// SDL driver did, so CheckParm and the like see no arguments, as before
int _argc;
char **_argv;
#include "fixedPoint_math.h"
#include "engine.h"
#include "network.h"
#include "draw.h"
#include "cache.h"
#include "SDL.h"
#include "duke-plat.h"

int32_t xres, yres, bytesperline, imageSize, maxpages;
uint8_t *frameplace;
uint8_t *frameoffset;
uint8_t *screen, vesachecked;
int32_t buffermode, origbuffermode, linearmode;
uint8_t permanentupdate = 0, vgacompatible;

static uint8_t *fb;			// the frame: xdim x ydim, 8 bits a pixel
static int fbw, fbh;
static int pal32[256];			// 0x00RRGGBB, for the display
uint8_t lastPalette[768];		// engine.c keeps the game's here too

static int32_t mouse_relative_x, mouse_relative_y;
static short mouse_buttons;
static int grabbed;
static unsigned int lastkey;
static uint8_t drawpixel_color;

int32_t total_render_time = 1;
int32_t total_rendered_frames = 0;
static int32_t last_render_ticks;

static int forcew, forceh;		// -res WxH
static int pages = 1;			// frames shown (_nextpage), for handle_events

// ---- the modes ----

// The sizes the game offers. The display scales any of them to the
// screen; the game draws a bigger one sharper, at a cost in time that
// grows with its pixels. 320x200 is what the DOS game ran in.
static int stdres[][2] = {
	{320, 200}, {320, 240}, {400, 300}, {512, 384},
	{640, 400}, {640, 480}, {800, 600}
};
enum { NSTDRES = 7 };

void getvalidvesamodes (void)
{
	int i;
	validmodecnt = 0;
	vidoption = 1;
	for (i = 0; i < NSTDRES; i++)
	{
		validmode[validmodecnt] = validmodecnt;
		validmodexdim[validmodecnt] = stdres[i][0];
		validmodeydim[validmodecnt] = stdres[i][1];
		validmodecnt++;
	}
}

static void init_new_res_vars (int32_t davidoption)
{
	int i, j;

	setupmouse ();
	xdim = xres = fbw;
	ydim = yres = fbh;
	bytesperline = fbw;
	vesachecked = 1;
	vgacompatible = 1;
	linearmode = 1;
	qsetmode = fbh;
	activepage = visualpage = 0;
	frameoffset = frameplace = fb;

	if (screen != NULL)
		screen = NULL;

	j = ydim * 4 * sizeof (int32_t);	// horizlookup and horizlookup2
	if (horizlookup)
		free (horizlookup);
	if (horizlookup2)
		free (horizlookup2);
	horizlookup = (int32_t *)malloc (j);
	horizlookup2 = (int32_t *)malloc (j);

	j = 0;
	for (i = 0; i <= ydim; i++)
	{
		ylookup[i] = j;
		j += bytesperline;
	}
	horizycent = (ydim * 4) >> 1;

	// drawrooms calls dosetaspect and works the rest out again
	oxyaspect = oxdimen = oviewingrange = -1;
	setBytesPerLine (bytesperline);
	setview (0L, 0L, xdim - 1, ydim - 1);
	setbrightness (curbrightness, palette);
	if (searchx < 0)
	{
		searchx = halfxdimen;
		searchy = ydimen >> 1;
	}
}

static void go_to_new_vid_mode (int davidoption, int w, int h)
{
	getvalidvesamodes ();
	if (fb == NULL || w != fbw || h != fbh)
	{
		if (fb)
			free (fb);
		fb = (uint8_t *)calloc (w * h, 1);
		if (fb == NULL)
			Error (EXIT_FAILURE, "duke3d: no memory for a %dx%d screen\n", w, h);
		fbw = w;
		fbh = h;
	}
	duke_plat_mode (w, h);
	init_new_res_vars (davidoption);
}

int _setgamemode (uint8_t davidoption, int32_t daxdim, int32_t daydim)
{
	int validated, i;

	if (forcew)
	{
		daxdim = forcew;
		daydim = forceh;
		forcew = 0;
	}
	getvalidvesamodes ();
	validated = 0;
	for (i = 0; i < validmodecnt; i++)
		if (validmodexdim[i] == daxdim && validmodeydim[i] == daydim)
			validated = 1;
	if (!validated)
	{
		printf ("duke3d: %dx%d is not one of the sizes; 320x200 instead\n", daxdim, daydim);
		daxdim = 320;
		daydim = 200;
	}
	go_to_new_vid_mode ((int)davidoption, daxdim, daydim);
	qsetmode = 200;
	last_render_ticks = getticks ();
	return 0;
}

void setvmode (int mode)
{
}

int screencapture (char *filename, uint8_t inverseit)
{
	return 0;
}

// ---- the platform's start ----

void _platform_init (int argc, char **argv, const char *title, const char *iconName)
{
	static int done;
	int i;
	if (done)
		return;
	done = 1;
	Setup_UnstableNetworking ();
	for (i = 1; i < argc; i++)
	{
		if (!strcmpi (argv[i], "-res") && i + 1 < argc)
		{
			char *x;
			forcew = atoi (argv[i + 1]);
			x = strchr (argv[i + 1], 'x');
			forceh = x ? atoi (x + 1) : 0;
			if (forcew <= 0 || forceh <= 0 || forcew > MAXXDIM || forceh > MAXYDIM)
				forcew = forceh = 0;
		}
	}
	duke_plat_init (argc, argv);
}

void _uninitengine (void)
{
	duke_quit ();
}

void duke_quit (void)
{
	duke_plat_close ();
}

// ---- the palette ----

static void present (void);

// Ken's format: 4 bytes an entry, blue, green, red (0-63), and one unused.
// A new palette is shown at once, with the frame last drawn, as SDL's 8-bit
// screen showed it: the game fades the screen in and out by the palette
// alone (palto), with no frame drawn between, and waits for a key on a
// screen it has faded in so (the end of an episode's)
int VBE_setPalette (uint8_t *palettebuffer)
{
	uint8_t *p;
	int i, r, g, b, v, changed;
	p = palettebuffer;
	changed = 0;
	for (i = 0; i < 256; i++)
	{
		b = p[0] & 63;
		g = p[1] & 63;
		r = p[2] & 63;
		v = (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 2) | (b >> 4));
		if (pal32[i] != v)
		{
			pal32[i] = v;
			changed = 1;
		}
		p += 4;
	}
	duke_sum_palette (palettebuffer);
	if (changed)
		present ();
	return 1;
}

int VBE_getPalette (int32_t start, int32_t num, uint8_t *palettebuffer)
{
	uint8_t *p;
	int i, v;
	p = palettebuffer + start * 4;
	for (i = start; i < start + num; i++)
	{
		v = pal32[i];
		*p++ = (v & 255) >> 2;
		*p++ = ((v >> 8) & 255) >> 2;
		*p++ = ((v >> 16) & 255) >> 2;
		*p++ = 0;
	}
	return 1;
}

void WriteLastPaletteToFile (void) {}
void WriteTranslucToFile (void) {}

// ---- the frame ----

static void present (void)
{
	if (fb)
		duke_plat_frame (fb, fbw, fbh, pal32);
}

void _updateScreenRect (int32_t x, int32_t y, int32_t w, int32_t h)
{
	present ();
}

void _nextpage (void)
{
	uint32_t ticks;
	pages++;
	_handle_events ();
	if (fb)
		duke_sum_frame (fb, fbw, fbh);
	present ();
	ticks = getticks ();
	total_render_time = ticks - last_render_ticks;
	if (total_render_time > 1000)
	{
		total_rendered_frames = 0;
		total_render_time = 1;
		last_render_ticks = ticks;
	}
	total_rendered_frames++;
}

void *_getVideoBase (void)
{
	return (void *)fb;
}

uint8_t readpixel (uint8_t *offset)
{
	return *offset;
}

void drawpixel (uint8_t *location, uint8_t pixel)
{
	*location = pixel;
}

void setcolor16 (uint8_t col)
{
	drawpixel_color = col;
}

void drawpixel16 (int32_t offset)
{
	if (fb && offset >= 0 && offset < fbw * fbh)
		fb[offset] = drawpixel_color;
}

void fillscreen16 (int32_t offset, int32_t color, int32_t blocksize)
{
	if (!fb)
		return;
	if (!pageoffset)
	{
		offset = offset << 3;
		offset += 640 * 336;
	}
	if (offset < 0)
		offset = 0;
	if (offset + blocksize > fbw * fbh)
		blocksize = fbw * fbh - offset;
	if (blocksize > 0)
		memset (fb + offset, (int)color, blocksize);
	_nextpage ();
}

// The 2D editor's lines; the game draws its map in its own way, so these
// are kept only so that the engine links. Bresenham, clipped to the frame.
void drawline16 (int32_t XStart, int32_t YStart, int32_t XEnd, int32_t YEnd, uint8_t Color)
{
	int dx, dy, sx, sy, err, e2;
	if (!fb)
		return;
	if (!pageoffset)
	{
		YStart += 336;
		YEnd += 336;
	}
	dx = XEnd > XStart ? XEnd - XStart : XStart - XEnd;
	dy = YEnd > YStart ? YStart - YEnd : YEnd - YStart;
	sx = XStart < XEnd ? 1 : -1;
	sy = YStart < YEnd ? 1 : -1;
	err = dx + dy;
	for (;;)
	{
		if (XStart >= 0 && XStart < fbw && YStart >= 0 && YStart < fbh && YStart < ydim16 + 336)
			fb[YStart * fbw + XStart] = Color;
		if (XStart == XEnd && YStart == YEnd)
			break;
		e2 = 2 * err;
		if (e2 >= dy)
		{
			err += dy;
			XStart += sx;
		}
		if (e2 <= dx)
		{
			err += dx;
			YStart += sy;
		}
	}
}

void clear2dscreen (void)
{
	int h;
	if (!fb)
		return;
	h = fbh;
	if (qsetmode == 350 && h > 350)
		h = 350;
	else if (qsetmode == 480 && ydim16 <= 336 && h > 336)
		h = 336;
	memset (fb, 0, fbw * h);
}

// ---- keys and the mouse ----

// A browser's key code as the PC's scan code; 0xE0nn for an extended key,
// 0 for one Build has no use for.
static unsigned int scancode (int code)
{
	static unsigned char letters[26] = {
		0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
		0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
	};
	if (code >= 65 && code <= 90)
		return letters[code - 65];
	if (code >= 49 && code <= 57)
		return 0x02 + (code - 49);
	if (code >= 112 && code <= 121)
		return 0x3B + (code - 112);
	if (code >= 96 && code <= 105)
	{
		static unsigned char pad[10] = { 0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49 };
		return pad[code - 96];
	}
	switch (code)
	{
	case 48: return 0x0B;
	case 27: return 0x01;
	case 173: case 189: return 0x0C;	// -
	case 61: case 187: return 0x0D;		// =
	case 8: return 0x0E;
	case 9: return 0x0F;
	case 219: return 0x1A;
	case 221: return 0x1B;
	case 13: return 0x1C;
	case 17: return 0x1D;
	case 59: case 186: return 0x27;
	case 222: return 0x28;
	case 192: return 0x29;
	case 16: return 0x2A;
	case 220: return 0x2B;
	case 188: return 0x33;
	case 190: return 0x34;
	case 191: return 0x35;
	case 106: return 0x37;
	case 18: return 0x38;
	case 32: return 0x39;
	case 20: return 0x3A;
	case 144: return 0x45;
	case 145: return 0x46;
	case 109: return 0x4A;
	case 107: return 0x4E;
	case 110: return 0x53;
	case 122: return 0x57;
	case 123: return 0x58;
	case 19: return 0x59;
	case 111: return 0xE035;
	case 36: return 0xE047;
	case 38: return 0xE048;
	case 33: return 0xE049;
	case 37: return 0xE04B;
	case 39: return 0xE04D;
	case 35: return 0xE04F;
	case 40: return 0xE050;
	case 34: return 0xE051;
	case 45: return 0xE052;
	case 46: return 0xE053;
	}
	return 0;
}

// a key to Build, as an interrupt would have given it
void duke_key (int code, int down)
{
	unsigned int sc, ext;
	sc = scancode (code);
	if (sc == 0)
		return;
	// Pause sends no release on a PC
	if (!down && sc == 0x59)
		return;
	ext = (sc & 0xFF00) >> 8;
	if (ext)
	{
		lastkey = ext;
		keyhandler ();
	}
	lastkey = (sc & 0xFF) + (down ? 0 : 128);
	keyhandler ();
}

// the mouse's buttons as Int 33h gives them, and the wheel as two more
void duke_mouse_button (int button, int down)
{
	int bit;
	bit = button == 0 ? 1 : button == 2 ? 2 : 4;
	if (down)
		mouse_buttons |= bit;
	else
		mouse_buttons &= ~bit;
}

void duke_mouse_wheel (int up)
{
	mouse_buttons |= up ? 8 : 16;
}

void duke_mouse_move (int dx, int dy)
{
	mouse_relative_x += dx;
	mouse_relative_y += dy;
}

int duke_grab (int mode)
{
	if (mode == SDL_GRAB_QUERY)
		return grabbed;
	grabbed = mode == SDL_GRAB_ON;
	duke_plat_grab (grabbed);
	return grabbed;
}

// A key or button let go in the frame it went down in is let go only after
// the whole of the next frame. The game asks what is down once a frame (its
// menus do), and a frame can take longer than a quick tap, whose press and
// release then both arrive inside it, perhaps after the asking: let go at
// once, the game would never have seen it down. (A DOS keyboard
// interrupted the game for each, which is what made that rare there.)
// Events are read several times a frame, so it is frames that count, not
// reads.
enum { HELD = 16 };
static int held[HELD], heldat[HELD], nheld;	// key codes, or 1000 + a button
static int downat[256 + 8];			// the frame each went down in

static void hold (int what, int at)
{
	if (nheld < HELD)
	{
		held[nheld] = what;
		heldat[nheld++] = at;
	}
	else if (what >= 1000)
		duke_mouse_button (what - 1000, 0);
	else
		duke_key (what, 0);
}

static void handle_events (void)
{
	int ev[4], i, n, code;
	duke_snd_poll ();
	duke_sum_input ();
	// down in frame k: still down all through frame k + 1, which has
	// begun when pages is k + 1, and let go once pages is k + 2
	for (i = n = 0; i < nheld; i++)
		if (heldat[i] < pages - 1)
		{
			if (held[i] >= 1000)
				duke_mouse_button (held[i] - 1000, 0);
			else
				duke_key (held[i], 0);
		}
		else
		{
			held[n] = held[i];
			heldat[n++] = heldat[i];
		}
	nheld = n;
	while (duke_plat_poll (ev))
	{
		switch (ev[0])
		{
		case DUKE_EV_KEYDOWN:
			downat[ev[1] & 255] = pages;
			duke_key (ev[1], 1);
			break;
		case DUKE_EV_KEYUP:
			code = ev[1] & 255;
			if (downat[code] >= pages - 1)
				hold (ev[1], downat[code]);
			else
				duke_key (ev[1], 0);
			break;
		case DUKE_EV_REL:
			if (grabbed)
				duke_mouse_move (ev[1], ev[2]);
			break;
		case DUKE_EV_DOWN:
			downat[256 + (ev[3] & 7)] = pages;
			duke_mouse_button (ev[3], 1);
			break;
		case DUKE_EV_UP:
			if (downat[256 + (ev[3] & 7)] >= pages - 1)
				hold (1000 + ev[3], downat[256 + (ev[3] & 7)]);
			else
				duke_mouse_button (ev[3], 0);
			break;
		case DUKE_EV_WHEEL:
			duke_mouse_wheel (ev[3] < 0);
			break;
		case DUKE_EV_QUIT:
			Error (EXIT_SUCCESS, "");
			break;
		}
	}
}

void _handle_events (void)
{
	handle_events ();
}

uint8_t _readlastkeyhit (void)
{
	return lastkey;
}

void initkeys (void) {}
void uninitkeys (void) {}

int setupmouse (void)
{
	mouse_relative_x = mouse_relative_y = 0;
	moustat = 1;
	if (!grabbed)
		duke_grab (SDL_GRAB_ON);
	return 1;
}

void readmousexy (short *x, short *y)
{
	if (x)
		*x = mouse_relative_x << 2;
	if (y)
		*y = mouse_relative_y << 2;
	mouse_relative_x = mouse_relative_y = 0;
}

void readmousebstatus (short *bstatus)
{
	if (bstatus)
		*bstatus = mouse_buttons;
	// the wheel: a click each time it turns
	mouse_buttons &= ~(8 | 16);
}

// no joystick
void _joystick_init (void) {}
void _joystick_deinit (void) {}
int _joystick_update (void) { return 0; }
int _joystick_axis (int axis) { return 0; }
int _joystick_hat (int hat) { return -1; }
int _joystick_button (int button) { return 0; }

void _idle (void)
{
	_handle_events ();
	duke_sleep (1);
}

// ---- the timer: Ken's idea, as display.c had it ----

static int timerticspersec;
static int32_t timerlastsample;

// milliseconds as timer ticks, in 32 bits
static int32_t ms_ticks (uint32_t ms)
{
	return (int32_t)((ms / 1000) * timerticspersec + (ms % 1000) * timerticspersec / 1000);
}
typedef void (*timercallback) (void);
static timercallback usertimercallback;

timercallback installusertimercallback (timercallback callback)
{
	timercallback old;
	old = usertimercallback;
	usertimercallback = callback;
	return old;
}

int inittimer (int tickspersecond)
{
	if (timerticspersec)
		return 0;
	timerticspersec = tickspersecond;
	timerlastsample = ms_ticks (duke_ms ());
	usertimercallback = NULL;
	return 0;
}

void uninittimer (void)
{
	timerticspersec = 0;
}

void sampletimer (void)
{
	int32_t n;
	// The game's waits for a sound to end (getpackets) come through here:
	// on DOS the card's interrupt ended it, here the poll does
	duke_snd_poll ();
	if (!timerticspersec)
		return;
	n = ms_ticks (duke_ms ()) - timerlastsample;
	if (n > 0)
	{
		totalclock += n;
		timerlastsample += n;
	}
	if (usertimercallback)
		for (; n > 0; n--)
			usertimercallback ();
}

uint32_t getticks (void)
{
	return duke_ms ();
}

int gettimerfreq (void)
{
	return timerticspersec;
}
