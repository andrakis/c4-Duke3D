// duke-c4ix.c -- on C4IX the display is the desktop's, which gives it way to
// the game until the game ends (include/display.h): duke-gui.c asks first.
// With -window, or when a program that does not give way has the display,
// the game plays in the Terminal window it was started from instead
// (window.h): 8-bit frames the desktop scales. A game asks to be served
// sooner and longer than the rest, and runs on CPU 0 unless it was pinned
// (the kernel's sched_softpin). As Quake's qg-c4ix.c.
//
// Two buffers: the desktop reads the one it was given when it next draws,
// and says so (WIN_FRAME); until then a newer frame waits in the other, and
// only the newest is sent. The game never waits on the window.
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "display.h"
#include "window.h"

int unice (int id, int nice);		// libc4ix (c4ix_user.h)
enum { DK_NICE = -5 };

int dk_take (void) { unice (0, DK_NICE); return udisplay_take (0); }
void dk_give (void) { udisplay_give (); }

static char *fbuf[2];
static int fsize[2];
static int fw[2], fh[2];
static int *fpal[2];
static int sent, waiting, pending, closed;

int dk_win_open (void)
{
	int i;
	unice (0, DK_NICE);
	for (i = 0; i < 2; i++)
		if (!(fpal[i] = (int *)calloc (256, sizeof (int))))
			return 0;
	return win_open ("Duke Nukem 3D");
}

static void send_frame (int b)
{
	win_fb (fbuf[b], fw[b], fh[b], fw[b], fpal[b]);
	win_present ();
	sent = b;
	waiting = 1;
}

void dk_win_draw (uint8_t *pixels, int w, int h, int *pal)
{
	int b;
	if (closed)
		return;
	// the buffer the desktop is not reading
	b = waiting ? 1 - sent : sent;
	if (fsize[b] < w * h)
	{
		free (fbuf[b]);
		if (!(fbuf[b] = (char *)malloc (w * h)))
		{
			fsize[b] = 0;
			return;
		}
		fsize[b] = w * h;
	}
	memcpy (fbuf[b], pixels, w * h);
	memcpy (fpal[b], pal, 256 * sizeof (int));
	fw[b] = w;
	fh[b] = h;
	if (waiting)
		pending = 1;
	else
		send_frame (b);
}

// the window's next key, as a browser keyCode and whether it went down:
// 1, 0 for none waiting, -1 once the window has closed
int dk_win_key (int *code, int *down)
{
	int ev[3], k;
	if (closed)
		return 0 - 1;
	while (1)
	{
		k = win_poll (ev);
		if (k == WIN_NONE)
			return 0;
		if (k < 0)
		{
			closed = 1;
			return 0 - 1;
		}
		if (k == WIN_FRAME)
		{
			// the desktop has drawn what it had: the newer frame, if any
			waiting = 0;
			if (pending)
			{
				pending = 0;
				send_frame (1 - sent);
			}
			continue;
		}
		if (k == WIN_KEY || k == WIN_KEYUP)
		{
			*code = ev[0];
			*down = k == WIN_KEY;
			return 1;
		}
	}
}

void dk_win_close (void)
{
	if (!closed)
	{
		win_close ();
		closed = 1;
	}
}
