# games/duke3d — Duke Nukem 3D on C4IX, C4DOS and the bare machine

3D Realms' Duke Nukem 3D (1996), as Chocolate Duke3D has it (the GPL
Atomic Edition source with Ken Silverman's Build engine, its assembly
rewritten in C), compiled by c4lc and played on libjs's whole display,
with its sound effects on libjs's sound card and its music on the card's
OPL, or, when the person chooses General MIDI music, as General MIDI on
the card's MIDI port. The shareware episode, L.A. Meltdown, ships with it; the full game
plays from the person's own `DUKE3D.GRP`. It is played from the keyboard
and the mouse.

## Where it runs

| where | how | platform |
|---|---|---|
| C4IX, the whole display | `duke3d`, or Go > Games > Duke Nukem 3D; the desktop gives way until it ends | `duke-gui.c`, `duke-c4ix.c` |
| C4IX, a window | `duke3d -window` in a Terminal; also when a program that does not give way has the display | `duke-gui.c`, `duke-c4ix.c` |
| C4DOS | `DUKE3D` at the prompt | `duke-gui.c`, `duke-bare.c` |
| a grin window, under C4DOS | as Doom's | `duke-gui.c`, `duke-bare.c` |
| the bare machine | `duke3d.c4r` as the machine's image; the page's `?system=duke3d` | `duke-gui.c`, `duke-bare.c` |

Keys are Duke's own, and the menus' (Options > Keys Setup) change them.
The mouse locks to the display at the first click, until Esc. It turns,
and walks forward and back, as DOS's mouse did; Options > Mouse Setup >
Mouse Aiming makes it look up and down instead. Its buttons are as DOS's
setup program left them: the left fires, the right strafes while it is
held and opens when it is double-clicked, the middle walks forward; and
the wheel changes weapons. Mouse Setup changes those too.

A key tapped faster than a frame is drawn still counts: a key let go in
the frame it went down in is let go after the next one.

## The full game: your own files

`duke3d -home` (Go > Games > Duke Nukem 3D (your files); the page's
`?system=duke3d-full` on the bare machine) plays your own `DUKE3D.GRP`:
from version 1.3D, 1.4 (Plutonium Pak) or 1.5 (Atomic Edition), which
the game tells apart by its CRC. Add it with the page's Uploads, then
start the game.

- On C4IX the uploads are in `/home`, and the game opens
  `/home/DUKE3D.GRP`.
- On the bare machine and under C4DOS the uploads are on the disk by
  their names, so the game opens `DUKE3D.GRP`.

The shareware episode is `DUKE3DSW.GRP` on c4's disks, so that your
`DUKE3D.GRP` never hides it: `duke3d` alone always plays the shareware.
Without your file, `duke3d -home` says what it wants and stops. The two
share their settings (`duke3d.cfg`) and saved games.

Duke Nukem 3D: 20th Anniversary World Tour's files are not one of these:
its `DUKE3D.GRP` has a fifth episode, new sounds and music, and CON
scripts that use commands the 1996 game does not have.

## Settings, and the screen size

Settings are changed in the game, as modern ports do, not with a separate
setup program: Options in the main menu (Esc) has Game Options, Keys
Setup, Mouse Setup, Sound Setup and Video Setup. They are kept in
`duke3d.cfg` where the game runs.

**The screen size** is Options > Video Setup > Resolution: Left and Right
choose, Enter applies it. The sizes are 320x200 (the default, as the DOS
game ran), 320x240, 400x300, 512x384, 640x400, 640x480 and 800x600. The
engine draws at the size chosen and the display scales it to fill the
screen, so a bigger size is sharper rather than smaller; the status bar,
the menus and the weapons are placed in 320x200's units and drawn
bigger. The time a frame takes grows with its pixels. `duke3d -res
640x480` starts at a size (and keeps it, as the menu does).

Video Setup's other lines are Detail (high or low), Screen Size (how
much of the screen the view takes beside the status bar), Brightness, and
Show FPS, the frame rate on screen. "Full screen" does nothing here: the
game always has the whole display.

## How fast

6.98 M VM instructions a frame at 320x200, playing the test's script on
E1L1 (`test-duke3d`), from 14.2 M in the first build, and 7.12 M playing
the Atomic Edition's first demo, which has more sloped floors: the inner
loops rewritten for c4's machine, and 64-bit arithmetic done in 32 bits,
with the same frames (c4's `docs/duke3d-port.md` §9). In the page, C4IX at
320x200 (2026-11-11, Chrome on a Chromebook): 12.9 fps as the page starts
(paced at 100 MHz, with the MMU); at `?mhz=400&cpus=1`, 16.2 fps with the
MMU and 20.3 without. A frame's cost grows with its pixels: 640x480 is
4.1 fps as the page starts.

## What is c4's here

Everything in `c4/`, and a few lines in Chocolate Duke3D's own files, each
under `#ifdef C4DUKE`:

- `duke-display.c`, in place of `Engine/src/display.c` (Ryan C. Gordon's
  SDL driver for Build), which it follows function by function: the
  8-bit frame and its palette, the mode list, keys as PC scan codes, the
  mouse's motion and buttons, the timer. A palette is shown as soon as
  it is set, as SDL's 8-bit screen showed it: the game fades in and out
  by the palette alone.
- `duke-gui.c`, `duke-c4ix.c`, `duke-bare.c`, `duke-host.c`: the
  platforms, as Quake's. Frames go out 8-bit, as `GUI_BLIT8` with their
  palette, scaled to a 4:3 display at least 640 wide.
- `duke-snd.c`, in place of audiolib's `fx_man.c` and `multivoc.c` (on
  SDL) and `sdl_midi.c`: Rise of the Triad's (c4's `games/rott`). Effects
  on the card's eight sample channels, which the host mixes; music by
  audiolib's own MIDI player and AdLib driver on the card's OPL, with the
  game's own timbre bank (`D3DTIMBR.TMB` in the GRP). When the card has
  a General MIDI synth on its MIDI port (the page's Configuration, Music:
  General MIDI; c4's `libjs/README.md`), the player's events go there
  instead, as they went to an MPU-401 and a Sound Canvas, and Apogee's
  extended MIDI picks each song's General MIDI tracks (`midi.c`).
  `duke-nosnd.c` is the silent one for gcc's build.
- `duke-sum.c`: the clock, one `rand` for every build, the GRP's name, the
  test's input, and every frame checksummed.
- `inc/`: the headers C4IX's libc lacks (`stdint.h`, `stdbool.h`,
  `inttypes.h`), DOS's for audiolib (`conio.h`, `dos.h`), and an `SDL.h`
  for the few SDL calls outside the display driver.

In Chocolate Duke3D's files:

- `draw.c`: the wall, masked-wall, floor and sloped-floor loops written
  again for c4's machine (a column at a time, locals, no unsigned `>>`,
  which c4lc makes a call, and no 64-bit products), and the per-pixel
  `pixelsAllowed` count, which never ran out, gone.
- `fixedPoint_math.h`, `.c`: `scale`, `mulscale` and `divscale` in 32
  bits where the value fits, and otherwise by long division and a product
  of 16-bit halves, with the same values as the `int64_t` code: c4 has no
  64-bit arithmetic, and each `int64_t` operation was a call. The
  `mulscaleN`, `dmulscaleN`, `tmulscaleN` and `divscaleN` wrappers are
  macros, one call where c4lc made two of them.
- `game.c`: the GRP is found by `duke-sum.c`; the program's own CRC
  (only for network games) is not taken; the CON file's name is an
  ordinary string (c4lc read `{"GAME.CON"}` as an empty one); a quit goes
  straight back to the desktop or the prompt, without the screens the
  game showed on its way out (the shareware's ordering information, the
  TEN advert), which waited for a key in a loop that never slept.
- `config.c`: 320x200 by default; the mouse's buttons as DOS's setup left
  them when `duke3d.cfg` says nothing of them; a test reads and writes no
  settings.
- `premap.c`: a level starting starts the test's input; the wait for the
  skill's sound to end lets the card be polled.
- `tiles.c`: the tile cache is at most 12 MB, as much as DOS gave it,
  where Chocolate takes the whole of the art's size; `setviewtotile` sets
  a row of the view a column of the tile apart, as Ken's BUILD did, so
  that low detail (Video Setup's Detail), which draws the view into a
  tile, no longer scrambles it.
- `animlib.c`: `CheckAnimStarted`, the test that an `.ANM` was loaded,
  made where it is needed instead of in a call.
- `filesystem.c`: the GRP's CRC is read through 64 KB from the heap, not
  a megabyte of BSS; `filelength` for c4.
- `global.c`: `FixFilePath` no longer writes a `/` over a name's end when
  it cannot open a directory (upstream's bug; a bare machine has none).
- `rts.c`: `malloc` for `alloca`, which c4lc has not.
- `platform.h`, `unix_compat.h`, `global.h`: `rand` is `duke_rand`; the
  byte order and the file modes C4IX's libc does not name.
- audiolib's `midi.c`, `al_midi.c`, `_midi.h`: as Rise of the Triad's —
  no wait on DOS's clock, the AdLib's tracks of an extended MIDI song, an
  AdLib whatever the card.

## Testing

- `make duke3d-data` takes 3D Realms' `3dduke13.zip` from `$DUKE3D_ZIP`
  or `data/MIRRORS`, by its SHA-256, and reads the game's files out of it
  (`data/README.md`). They are never committed.
- `make test-duke3d` plays the first level from a fixed script of keys,
  mouse moves and a mouse button, on a clock that moves only as the game
  runs, built by gcc and by c4lc, on the bare machine, under C4DOS and
  inside C4IX: all must show the same frames, to the checksum of every
  pixel and the palette. Then, inside C4IX, the screen size changed in
  the game's menu (Options, Video Setup) from 320x200 to 400x300 and the
  detail set low, and a quit from the game (F10, Y), which must end it. The
  shareware's own demos were recorded by version 1.3D, which this code
  does not play.
- `duke3d -sum N -frames N -script -shot N` are the test's: a checksum
  line every N frames, a stop, the script (`-menu` for the menu's), and
  every Nth frame written as a PPM, to see what the test plays.
