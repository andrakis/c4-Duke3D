// duke-gui.c -- the port's platform on libjs's display (libjs/guest/gui.h):
// C4IX, where the desktop gives the display way until the game ends
// (duke-c4ix.c), and the bare machine and C4DOS (duke-bare.c).
//
// Frames go out 8-bit, as GUI_BLIT8 with their palette, scaled to a 4:3
// display at least 640 wide, so that every size the game offers fills the
// screen. Keys and the mouse come from the display's event ring; the mouse
// is locked to the display from the first click until Esc (GUI_PTR_LOCK),
// and moves by its motion. A test (-sum) runs without the display unless
// -show is given too, so that its time counts the drawing.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "duke-plat.h"
#include "gui.h"

int  dk_take (void);			// the display is ours to use (duke-c4ix.c, duke-bare.c)
void dk_give (void);
int  dk_win_open (void);		// or a window (C4IX's Terminal, grin's)
void dk_win_draw (uint8_t *pixels, int w, int h, int *pal);
int  dk_win_key (int *code, int *down);
void dk_win_close (void);

enum { RING = 65536 };

static int on;			// the display is ours
static int win;			// or the game plays in a window
static int *ring;
static int cw, ch;		// the display's size

static void dk_close (void)
{
	if (!on)
		return;
	gui_cursor (GUI_PTR_ARROW);
	gui_detach ();
	dk_give ();
	on = 0;
}

static void dk_unwin (void)
{
	if (win)
		dk_win_close ();
	win = 0;
}

static int dk_window (void)
{
	if (!(win = dk_win_open ()))
		return 0;
	atexit (dk_unwin);
	return 1;
}

void duke_plat_init (int argc, char **argv)
{
	int i, want;
	if (!duke_showing)
		return;
	want = 0;
	for (i = 1; i < argc; i++)
		if (!strcmp (argv[i], "-window") || !strcmp (argv[i], "--window"))
			want = 1;
	if (want && dk_window ())
		return;
	if (!gui_present ())
	{
		printf ("duke3d: this machine has no display\n");
		return;
	}
	if (!dk_take ())
	{
		if (dk_window ())
		{
			printf ("duke3d: a program has the display; playing in the window\n");
			return;
		}
		printf ("duke3d: a program has the display\n");
		return;
	}
	cw = 640;
	ch = 480;
	if (!(ring = (int *)malloc (RING)) || !gui_attach (ring, RING, cw, ch))
	{
		dk_give ();
		return;
	}
	gui_events (GUI_M_KEYS | GUI_M_MOVE | GUI_M_BUTTONS | GUI_M_WINDOW | GUI_M_WHEEL | GUI_M_REL);
	gui_cursor (GUI_PTR_LOCK);
	gui_clear (0x000000);
	gui_show ();
	on = 1;
	atexit (dk_close);
}

// the game's screen is w x h: a display of 4:3, at least 640 wide
void duke_plat_mode (int w, int h)
{
	int nw, nh;
	if (!on)
		return;
	nw = w < 640 ? 640 : w;
	nh = nw * 3 / 4;
	if (nw != cw || nh != ch)
	{
		cw = nw;
		ch = nh;
		gui_size (cw, ch);
	}
	gui_clear (0x000000);
}

void duke_plat_frame (uint8_t *pixels, int w, int h, int *pal)
{
	if (win)
	{
		dk_win_draw (pixels, w, h, pal);
		return;
	}
	if (!on)
		return;
	gui_blit8 (0, 0, w, h, (char *)pixels, w, pal, cw, ch);
	gui_show ();
}

int duke_plat_poll (int *ev)
{
	int k, code, down;
	if (win)
	{
		k = dk_win_key (&code, &down);
		if (k < 0)
		{
			// the window closed: the game ends, as when it is quit
			win = 0;
			ev[0] = DUKE_EV_QUIT;
			return 1;
		}
		if (k == 0)
			return 0;
		ev[0] = down ? DUKE_EV_KEYDOWN : DUKE_EV_KEYUP;
		ev[1] = code;
		ev[2] = ev[3] = 0;
		return 1;
	}
	if (!on)
		return 0;
	while (gui_poll (ev))
	{
		switch (ev[0])
		{
		case GUI_EV_KEYDOWN:
		case GUI_EV_KEYUP:
		case GUI_EV_REL:
		case GUI_EV_DOWN:
		case GUI_EV_UP:
		case GUI_EV_WHEEL:
			return 1;
		}
	}
	return 0;
}

void duke_plat_grab (int grab)
{
	if (on)
		gui_cursor (grab ? GUI_PTR_LOCK : GUI_PTR_ARROW);
}

void duke_plat_close (void)
{
	dk_unwin ();
	dk_close ();
}

uint32_t duke_plat_ms (void)
{
	return (uint32_t)__time ();
}

void duke_plat_sleep (int ms)
{
	if (ms > 0)
		__c4_usleep (ms * 1000);
}
