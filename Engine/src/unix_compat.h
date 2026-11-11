//
//  unix_compat.h
//  Duke3D
//
//  Based on macos_compat.h
//  Copyright Wed, Jul 31, 2013, Juan Manuel Borges Caño (GPLv3+)
//

#ifndef Duke3D_unix_compat_h
#define Duke3D_unix_compat_h

//#define BYTE_ORDER LITTLE_ENDIAN
#define PLATFORM_SUPPORTS_SDL

#include <stdlib.h>

#define kmalloc(x) malloc(x)
#define kkmalloc(x) malloc(x)
#define kfree(x) free(x)
#define kkfree(x) free(x)

#ifdef FP_OFF
#undef FP_OFF
#endif

// Horrible horrible macro: Watcom allowed memory pointer to be cast
// to a 32bits integer. The code is unfortunately stuffed with this :( !
#define FP_OFF(x) ((int32_t) (x))

#ifndef max
#define max(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef min
#define min(x, y)  (((x) < (y)) ? (x) : (y))
#endif

#include <inttypes.h>
#define __int64 int64_t

#define O_BINARY 0

#define UDP_NETWORKING 1

#define PLATFORM_UNIX 1

/*
#define SOL_IP SOL_SOCKET
#define IP_RECVERR  SO_BROADCAST
*/

#define stricmp strcasecmp
#define strcmpi strcasecmp

#define S_IREAD S_IRUSR
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

#define USER_DUMMY_NETWORK 1

#ifdef C4DUKE
/* c4's port: what glibc's headers say and C4IX's libc does not -- the byte
   order (a PC's, little-endian), and the file modes for the group and the
   rest, which mean nothing on a machine of one user */
#ifndef BYTE_ORDER
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#ifndef S_IRGRP
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#endif
#ifndef S_IRWXU
#define S_IRWXU 0700
#endif
#endif

#ifdef __MINGW32__
// MinGW, the 32-bit host build on Windows (c4's docs/windows.md),
// declares these itself, with char * where Duke's take uint8_t *. Its
// declarations are behind us (the headers above); Duke's own are renamed
// past them.
#define strlwr     duke_strlwr
#define strupr     duke_strupr
#define itoa       duke_itoa
#define ltoa       duke_ltoa
#define ultoa      duke_ultoa
#define filelength duke_filelength
#endif

#endif
