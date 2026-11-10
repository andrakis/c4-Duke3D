// dos.h -- audiolib includes it for DOS's interrupt calls, which the parts
// of it here (midi.c, al_midi.c) do not make; and for Watcom's
// calling-convention keyword, and its min and max
#ifndef C4DUKE_DOS_H
#define C4DUKE_DOS_H 1
#ifndef cdecl
#define cdecl
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#define _enable()
#endif
