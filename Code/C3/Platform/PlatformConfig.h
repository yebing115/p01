#pragma once

#define WINDOWS_PLATFORM 1
#define IOS_PLATFORM 2
#define PS4_PLATFORM 3

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM WINDOWS_PLATFORM
#elif defined(__ORBIS__)
#define PLATFORM PS4_PLATFORM
#endif

#define ON_WINDOWS (PLATFORM == WINDOWS_PLATFORM)
#define ON_IOS (PLATFORM == IOS_PLATFORM)
#define ON_PS4 (PLATFORM == PS4_PLATFORM)

#if ON_WINDOWS
#define SYSTEM_ENCODING UTF_16
#else
#define SYSTEM_ENCODING UTF_8
#endif

//////////////////////////////////////////////////////////////////////////
#define C3_MAX_VIEWS                UINT8_C(127)
#define C3_MAX_VIEW_NAME            256
#define C3_VIEW_NAME_RESERVED       6
#define C3_MAX_DRAW_CALLS           (16 << 10)
#define C3_MAX_CONSTANT_BUFER_SIZE  (16 << 10)
#define C3_MAX_SHADERS 128
#define C3_MAX_PROGRAMS 64
#define C3_MAX_CONSTANTS 512
#define C3_MAX_TEXTURE_SAMPLERS 16
#define C3_MAX_TEXTURES (16 << 10)
#define C3_MAX_FRAME_BUFFERS 64
#define C3_MAX_VERTEX_DECLS 16
#define C3_MAX_VERTEX_BUFFERS (8 << 10)
#define C3_MAX_INDEX_BUFFERS (8 << 10)
#define C3_MAX_DYNAMIC_VERTEX_BUFFERS (4 << 10)
#define C3_MAX_DYNAMIC_INDEX_BUFFERS (4 << 10)
#define C3_MAX_COLOR_PALETTE 16
#define C3_MAX_MATRIX_CACHE (C3_MAX_DRAW_CALLS + 1)
#define C3_MAX_RECT_CACHE (16 << 10)
#define C3_MAX_COMMAND_BUFFER_SIZE (64 << 10)

#define C3_RESOLUTION_DEFAULT_WIDTH 1280
#define C3_RESOLUTION_DEFAULT_HEIGHT 720
#define C3_RESOLUTION_DEFAULT_FLAGS C3_RESET_NONE
#define C3_TRANSIENT_INDEX_BUFFER_SIZE (2 << 16)
#define C3_TRANSIENT_VERTEX_BUFFER_SIZE (6 << 20)

//////////////////////////////////////////////////////////////////////////
#define C3_MAX_ASSETS 4096
#define C3_MAX_JOBS 2048
#define C3_MAX_WORKER_THREADS 8
#define C3_MAX_FIBERS 256

//////////////////////////////////////////////////////////////////////////
#define C3_MAX_ENTITIES           (10 << 10)
#define C3_MAX_CAMERAS            8
#define C3_MAX_MODEL_RENDERERS    (10 << 10)
#define C3_MAX_TRANSFORMS         (10 << 10)
#define C3_MAX_LIGHTS             (10 << 10)
