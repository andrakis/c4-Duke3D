// duke-sum.c -- what every build of c4's port shares besides the display
// driver: the clock, one rand for every build, and the test.
//
// The shareware's demos were recorded by version 1.3D, which the Atomic
// Edition's code does not play (their version byte is 0). So the test is a
// played game instead: -sum starts a run on a clock that moves only as the
// game runs, and -script plays a fixed set of key presses and mouse
// moves, frame by frame, from the moment the level starts. Every frame is
// checksummed with its palette, and gcc's build and c4lc's, on the bare
// machine, under C4DOS and inside C4IX, must show the same frames.
//
//   -sum [N]     the test's clock; a line every N frames, and the sum at the end
//   -frames N    stop after N frames
//   -script      play the test's keys and mouse
//   -menu        or change the screen size in the game's menu (Video Setup)
//   -show        and show the frames, so that a run's time counts the drawing
//   -shot N      every Nth frame as shotN.ppm
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "duke-plat.h"

int duke_testing;
int duke_showing;
static int dosum, stopat, script, home, shot, menu;
static int frames;
static uint32_t sum, virt_ms, ms0;
static uint8_t pal[1024];

// ---- rand: the same in every build (compiled as -Drand=duke_rand) ----

static uint32_t rand_s = 1;
int duke_rand (void)
{
	rand_s = rand_s * 1103515245u + 12345u;
	return (rand_s >> 16) & 0x7fff;
}

void duke_srand (unsigned s)
{
	rand_s = s;
}

// ---- the clock ----

// A test's clock moves a millisecond each time it is read, which is the
// same in every build, and a frame's time at each frame (a game tic is
// 1/30 s): the game then runs about a tic a frame, whatever the machine.
uint32_t duke_ms (void)
{
	if (duke_testing)
		return ++virt_ms;
	return duke_plat_ms ();
}

void duke_sleep (int ms)
{
	if (duke_testing)
		virt_ms += ms;
	else
		duke_plat_sleep (ms);
}

// ---- the command line, before the game reads it ----

static void quitted (void);

void duke_sum_args (int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp (argv[i], "-sum"))
		{
			dosum = i + 1 < argc && argv[i + 1][0] >= '0' && argv[i + 1][0] <= '9' ? atoi (argv[i + 1]) : 1;
			duke_testing = 1;
			atexit (quitted);
		}
		else if (!strcmp (argv[i], "-frames") && i + 1 < argc)
			stopat = atoi (argv[i + 1]);
		else if (!strcmp (argv[i], "-script"))
			script = 1;
		else if (!strcmp (argv[i], "-menu"))
		{
			script = 1;
			menu = 1;
		}
		else if (!strcmp (argv[i], "-show"))
			duke_showing = 1;
		else if (!strcmp (argv[i], "-home"))
			home = 1;
		else if (!strcmp (argv[i], "-shot") && i + 1 < argc)
			shot = atoi (argv[i + 1]);
	}
	if (!duke_testing)
		duke_showing = 1;
	// On C4IX the game lives in /usr/games/duke3d, and starts there; on the
	// bare machine and under C4DOS there is only the one directory, and the
	// chdir fails.
	chdir ("/usr/games/duke3d");
}

// ---- the GRP ----

// The shareware episode is DUKE3DSW.GRP on c4's disks, so that the
// person's own DUKE3D.GRP never hides it. With -home the game plays theirs
// alone, added with the page's Uploads: in the folder they can be put in
// for Duke first, then in /home -- on C4IX /home/duke3d and /home, on the
// bare machine and under C4DOS duke3d/ at the disk's root, and the root.

static int found (char *name)
{
	FILE *f;
	if (!(f = fopen (name, "rb")))
		return 0;
	fclose (f);
	return 1;
}

void duke_find_grp (char *path)
{
	// C4IX has a /home; the bare machine and C4DOS do not, and there the
	// host finds "/home/X" it does not have by its last part, at the root
	static char *mine[] = { "/home/duke3d/duke3d.grp", "/home/duke3d/DUKE3D.GRP",
				"/home/DUKE3D.GRP", "/home/duke3d.grp", "/home/Duke3D.grp",
				"DUKE3D.GRP", "duke3d.grp", 0 };
	static char *disk[] = { "duke3d/duke3d.grp", "duke3d/DUKE3D.GRP", "DUKE3D.GRP", "duke3d.grp", 0 };
	static char *ours[] = { "DUKE3DSW.GRP", "duke3dsw.grp", 0 };
	struct stat st;
	char **n;
	for (n = !home ? ours : stat ("/home", &st) == 0 ? mine : disk; *n; n++)
		if (found (*n))
		{
			strcpy (path, *n);
			printf ("duke3d: playing %s\n", path);
			return;
		}
	if (home)
		printf ("duke3d -home plays your own DUKE3D.GRP, from Duke Nukem 3D 1.3D, 1.4 or 1.5\n"
			"(Atomic Edition): add it with the page's Uploads (in /home/duke3d or /home),\n"
			"then start it again.\n");
	else
		printf ("duke3d: the shareware episode, DUKE3DSW.GRP, is not here\n");
	path[0] = 0;
}

// ---- the test's input ----

// From the frame the level is first drawn: [first, last) a key held (a
// browser key code), or the mouse moved each frame by dx, dy, or a button.
enum { S_KEY = 1, S_MOUSE = 2, S_BUTTON = 3 };
static int steps[][5] = {
	{ S_KEY,     20,  90, 38, 0 },	// up: walk
	{ S_KEY,     90, 105, 37, 0 },	// left: turn
	{ S_KEY,    105, 160, 38, 0 },
	{ S_KEY,    140, 150, 17, 0 },	// ctrl: fire
	{ S_KEY,    160, 190, 39, 0 },	// right
	{ S_KEY,    170, 230, 38, 0 },
	{ S_KEY,    230, 236, 32, 0 },	// space: open
	{ S_MOUSE,  240, 270, 12, 0 },	// the mouse turns
	{ S_MOUSE,  270, 280, 0, -6 },	// and looks up
	{ S_BUTTON, 280, 290, 0, 0 },	// and fires
	{ S_KEY,    290, 330, 40, 0 },	// down: back
	{ S_KEY,    300, 302, 50, 0 },	// 2: the pistol's ammo (a weapon key)
	{ S_KEY,    330, 400, 38, 0 },
	{ S_KEY,    335, 340, 16, 0 },	// shift: run
	{ S_KEY,    350, 380, 65, 0 },	// a: jump
	{ S_KEY,    380, 400, 90, 0 },	// z: crouch
	{ S_KEY,    400, 460, 37, 0 },
	{ S_KEY,    410, 440, 17, 0 },
	{ 0, 0, 0, 0, 0 }
};
// -menu: the screen size changed in the game, as a player does it: Esc,
// Options (the menu's fourth line), Video Setup (its fifth), the size one
// to the right twice (320x200 to 400x300), Enter; then Detail (two lines
// down) to low, and out again; then a walk, and a quit from the game (F10,
// Y), which must end the program
static int menusteps[][5] = {
	{ S_KEY,  20,  22, 27, 0 },	// esc: the menu
	{ S_KEY,  30,  32, 40, 0 },	// down
	{ S_KEY,  36,  38, 40, 0 },
	{ S_KEY,  42,  44, 40, 0 },
	{ S_KEY,  50,  52, 13, 0 },	// enter: options
	{ S_KEY,  60,  62, 40, 0 },
	{ S_KEY,  66,  68, 40, 0 },
	{ S_KEY,  72,  74, 40, 0 },
	{ S_KEY,  78,  80, 40, 0 },
	{ S_KEY,  86,  88, 13, 0 },	// enter: video setup
	{ S_KEY,  96,  98, 39, 0 },	// right: the next size
	{ S_KEY, 104, 106, 39, 0 },
	{ S_KEY, 112, 114, 13, 0 },	// enter: that size
	{ S_KEY, 120, 122, 40, 0 },	// down, down: detail
	{ S_KEY, 126, 128, 40, 0 },
	{ S_KEY, 134, 136, 13, 0 },	// enter: low
	{ S_KEY, 144, 146, 27, 0 },	// esc, esc, esc
	{ S_KEY, 154, 156, 27, 0 },
	{ S_KEY, 164, 166, 27, 0 },
	{ S_KEY, 174, 234, 38, 0 },	// up: walk, at the new size and low detail
	{ S_KEY, 244, 246, 121, 0 },	// F10: quit?
	{ S_KEY, 254, 256, 89, 0 },	// y: yes
	{ 0, 0, 0, 0, 0 }
};
static int level0 = -1;		// the frame the level was first drawn

void duke_sum_level (void)
{
	if (level0 < 0)
		level0 = frames;
}

void duke_sum_input (void)
{
	int i, f, *p;
	static int last = -1;
	if (!script || level0 < 0)
		return;
	f = frames - level0;
	if (f == last)
		return;
	last = f;
	// five words a step: what, first, last, key or dx, dy
	p = menu ? &menusteps[0][0] : &steps[0][0];
	for (i = 0; p[i * 5]; i++)
	{
		if (p[i * 5 + 0] == S_KEY)
		{
			if (f == p[i * 5 + 1])
				duke_key (p[i * 5 + 3], 1);
			if (f == p[i * 5 + 2])
				duke_key (p[i * 5 + 3], 0);
		}
		else if (p[i * 5 + 0] == S_MOUSE)
		{
			if (f >= p[i * 5 + 1] && f < p[i * 5 + 2])
				duke_mouse_move (p[i * 5 + 3], p[i * 5 + 4]);
		}
		else if (p[i * 5 + 0] == S_BUTTON)
		{
			if (f == p[i * 5 + 1])
				duke_mouse_button (0, 1);
			if (f == p[i * 5 + 2])
				duke_mouse_button (0, 0);
		}
	}
}

// ---- the frames ----

void duke_sum_palette (uint8_t *palette)
{
	memcpy (pal, palette, 1024);
}

static int ended;

static void done (void)
{
	uint32_t ms;
	ended = 1;
	ms = duke_plat_ms () - ms0;
	if (!ms)
		ms = 1;
	printf ("speed: %d frames in %d.%03d s, %d.%d fps\n", frames, (int)(ms / 1000), (int)(ms % 1000),
		(int)(frames * 1000 / ms), (int)(frames * 10000 / ms % 10));
	printf ("timedemo: %d frames, checksum %08x\n", frames, sum);
	duke_plat_close ();
	exit (0);
}

// a test's game that ended itself, by its menu's quit (-menu does)
static void quitted (void)
{
	if (ended)
		return;
	ended = 1;
	printf ("timedemo: %d frames, checksum %08x\n", frames, sum);
	printf ("duke3d: quit from the game\n");
}

void duke_sum_frame (uint8_t *pixels, int w, int h)
{
	uint32_t s, *p;
	int i, n;
	if (!frames)
		ms0 = duke_plat_ms ();
	++frames;
	if (duke_testing)
		virt_ms += 33;
	if (dosum)
	{
		s = sum;
		p = (uint32_t *)pixels;
		n = w * h / 4;
		for (i = 0; i < n; i++)
			s = s * 31 + p[i];
		p = (uint32_t *)pal;
		for (i = 0; i < 256; i++)
			s = s * 31 + (p[i] & 0x3f3f3f);
		s = s * 31 + w * 65536 + h;
		sum = s;
		if (dosum > 1 && frames % dosum == 0)
			printf ("sum: frame %d %08x\n", frames, s);
	}
	// -shot N: frame N as shotN.ppm, to see what a test plays
	if (shot && frames % shot == 0)
	{
		char name[32];
		FILE *f;
		sprintf (name, "shot%d.ppm", frames);
		if ((f = fopen (name, "wb")))
		{
			fprintf (f, "P6\n%d %d\n63\n", w, h);
			for (i = 0; i < w * h; i++)
			{
				fputc (pal[pixels[i] * 4 + 2], f);
				fputc (pal[pixels[i] * 4 + 1], f);
				fputc (pal[pixels[i] * 4], f);
			}
			fclose (f);
		}
	}
	if (stopat && frames >= stopat)
		done ();
}
