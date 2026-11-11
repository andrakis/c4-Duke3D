#ifndef _CVARDEFS_H_
#define _CVARDEFS_H_

#ifdef _MSC_VER   // MSVC lacked inttypes.h; MinGW has its own
   #include "../../Engine/src/windows/inttypes.h"
#else
   #include <inttypes.h>
#endif

void CVARDEFS_Init();
void CVARDEFS_Render();
//
// Function declarations
//
void CVARDEFS_DefaultFunction(void* var);
void CVARDEFS_FunctionQuit(void* var);
void CVARDEFS_FunctionClear(void* var);
void CVARDEFS_FunctionLevel(void* var);
void CVARDEFS_FunctionName(void* var);
void CVARDEFS_FunctionPlayMidi(void* var);
void CVARDEFS_FunctionFOV(void* var);
void CVARDEFS_FunctionTickRate(void* var);
void CVARDEFS_FunctionTicksPerFrame(void* var);
void CVARDEFS_FunctionHelp(void* var);

//
// Variable declarations
//
#ifdef C4DUKE
// c4: declared here, and defined once (cvar_defs.c). c4lc and c4rlink give
// every unit that includes a definition its own copy, where gcc's -fcommon
// merges them into one
#define CVAR_DEFINE extern
#else
#define CVAR_DEFINE
#endif
CVAR_DEFINE int g_CV_console_text_color;
CVAR_DEFINE int g_CV_num_console_lines;
CVAR_DEFINE int g_CV_classic;
CVAR_DEFINE int g_CV_TransConsole;
CVAR_DEFINE int g_CV_DebugJoystick;
CVAR_DEFINE int g_CV_DebugSound;
CVAR_DEFINE int g_CV_DebugFileAccess;
CVAR_DEFINE uint32_t sounddebugActiveSounds;
CVAR_DEFINE uint32_t sounddebugAllocateSoundCalls;
CVAR_DEFINE uint32_t sounddebugDeallocateSoundCalls;


CVAR_DEFINE int g_CV_CubicInterpolation;

#endif