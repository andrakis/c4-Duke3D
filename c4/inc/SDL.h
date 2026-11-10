// SDL.h -- the few SDL calls Chocolate Duke3D makes outside its display
// driver, which c4's port replaces (c4/duke-display.c): the menu's mouse
// grab (menues.c), its full-screen toggle, and Error's SDL_Quit
// (global.c). Built by c4lc for C4IX, C4DOS and the bare machine, and by
// gcc for the host.
#ifndef C4DUKE_SDL_H
#define C4DUKE_SDL_H 1

typedef struct SDL_Surface SDL_Surface;

#define SDL_INIT_VIDEO 0x20
#define SDL_GRAB_QUERY (-1)
#define SDL_GRAB_OFF   0
#define SDL_GRAB_ON    1

int  duke_grab (int mode);		// duke-display.c: the mouse locked to the display
void duke_quit (void);			// and the display given back

#define SDL_WM_GrabInput(m)  duke_grab (m)
#define SDL_ShowCursor(on)   0
#define SDL_QuitSubSystem(f) 0
#define SDL_Quit()           duke_quit ()

#endif
