// duke-plat.h -- the layers of c4's port under the Build engine:
//
//   duke-display.c  Build's display driver, the same in every build
//   duke-sum.c      the clock, a test's input and every frame's checksum
//   duke-gui.c      libjs's display, with duke-c4ix.c or duke-bare.c
//   duke-host.c     no display: gcc's build, the reference
#ifndef C4DUKE_PLAT_H
#define C4DUKE_PLAT_H 1

#include <stdint.h>

// events, as libjs's display numbers them (gui.h), and one of its own
enum {
	DUKE_EV_DOWN = 2, DUKE_EV_UP = 3, DUKE_EV_KEYDOWN = 4, DUKE_EV_KEYUP = 5,
	DUKE_EV_WHEEL = 8, DUKE_EV_REL = 9, DUKE_EV_QUIT = 100
};

// the platform: duke-gui.c or duke-host.c
void     duke_plat_init (int argc, char **argv);
void     duke_plat_mode (int w, int h);		// the game's screen is now w x h
void     duke_plat_frame (uint8_t *pixels, int w, int h, int *pal);
int      duke_plat_poll (int *ev);			// 1 and ev[0..3], or 0 for none
void     duke_plat_grab (int on);			// the mouse locked to the display
void     duke_plat_close (void);
uint32_t duke_plat_ms (void);
void     duke_plat_sleep (int ms);

// duke-sum.c
extern int duke_testing;				// -sum: a test, on its own clock
extern int duke_showing;				// the frames go to the display
void     duke_sum_args (int argc, char **argv);
void     duke_sum_level (void);			// a level is being played
uint32_t duke_ms (void);
void     duke_sleep (int ms);
void     duke_sum_frame (uint8_t *pixels, int w, int h);
void     duke_sum_palette (uint8_t *palette);
void     duke_sum_input (void);

// duke-snd.c (or duke-nosnd.c): sounds let go, and the music run ahead
void     duke_snd_poll (void);

// duke-display.c
void     duke_key (int code, int down);		// a browser key code
void     duke_mouse_button (int button, int down);	// 0 left, 1 middle, 2 right
void     duke_mouse_wheel (int up);
void     duke_mouse_move (int dx, int dy);
void     duke_quit (void);

#endif
