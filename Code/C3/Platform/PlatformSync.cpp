#include "C3PCH.h"
#include "PlatformSync.h"

thread_local u32 g_thread_id = 0;
u32 MAIN_THREAD_ID = MAKE_FOURCC('M', 'A', 'I', 'N');
u32 RENDER_THREAD_ID = MAKE_FOURCC('R', 'N', 'D', 'E');
