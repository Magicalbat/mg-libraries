/*
MGP Header
=================================================
  __  __  ___ ___   _  _ ___   _   ___  ___ ___ 
 |  \/  |/ __| _ \ | || | __| /_\ |   \| __| _ \
 | |\/| | (_ |  _/ | __ | _| / _ \| |) | _||   /
 |_|  |_|\___|_|   |_||_|___/_/ \_\___/|___|_|_\

=================================================
*/

#ifndef MG_PROFILE_H
#define MG_PROFILE_H

#ifndef MGP_FUNC_DEF
#   if defined(MGP_STATIC)
#      define MGP_FUNC_DEF static
#   elif defined(_WIN32) && defined(MGP_DLL) && defined(MG_ARENA_IMPL)
#       define MGP_FUNC_DEF __declspec(dllexport)
#   elif defined(_WIN32) && defined(MGP_DLL)
#       define MGP_FUNC_DEF __declspec(dllimport)
#   else
#      define MGP_FUNC_DEF extern
#   endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int8_t   mgp_i8;
typedef int16_t  mgp_i16;
typedef int32_t  mgp_i32;
typedef int64_t  mgp_i64;
typedef uint8_t  mgp_u8;
typedef uint16_t mgp_u16;
typedef uint32_t mgp_u32;
typedef uint64_t mgp_u64;

typedef mgp_i32 mgp_b32;

typedef enum {
    MGP_SEC = 0,
    MGP_MILLI_SEC = 1,
    MGP_MICRO_SEC = 2,
    MGP_NANO_SEC = 3,
    MGP_TIMEUNIT_COUNT
} mgp_time_unit;

MGP_FUNC_DEF void mgp_init(void);

MGP_FUNC_DEF mgp_u64 mgp_gettime_s(void);
MGP_FUNC_DEF mgp_u64 mgp_gettime_ms(void);
MGP_FUNC_DEF mgp_u64 mgp_gettime_us(void);
MGP_FUNC_DEF mgp_u64 mgp_gettime_ns(void);
MGP_FUNC_DEF mgp_u64 mgp_gettime(mgp_time_unit unit);

#ifdef __cplusplus
}
#endif

#endif // MG_PROFILE_H

/*
MGP Implementation
=========================================================================================
  __  __  ___ ___   ___ __  __ ___ _    ___ __  __ ___ _  _ _____ _ _____ ___ ___  _  _ 
 |  \/  |/ __| _ \ |_ _|  \/  | _ \ |  | __|  \/  | __| \| |_   _/_\_   _|_ _/ _ \| \| |
 | |\/| | (_ |  _/  | || |\/| |  _/ |__| _|| |\/| | _|| .` | | |/ _ \| |  | | (_) | .` |
 |_|  |_|\___|_|   |___|_|  |_|_| |____|___|_|  |_|___|_|\_| |_/_/ \_\_| |___\___/|_|\_|

=========================================================================================
*/

#ifdef MG_PROFILE_IMPL

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__)
#    define MGP_PLATFORM_LINUX
#elif defined(__APPLE__)
#    define MGP_PLATFORM_APPLE
#elif defined(_WIN32)
#    define MGP_PLATFORM_WIN32
#else
#   error "MGP: Unknown platform"
#endif

#if defined(MGP_PLATFORM_LINUX) || defined(MGP_PLATFORM_APPLE)

#   error "TODO"
// https://gist.github.com/jbenet/1087739

#elif defined(MGP_PLATFORM_WIN32)

#pragma comment(lib, "winmm.lib")

#ifndef UNICODE
#    define UNICODE
#endif
#include <Windows.h>

static mgp_u64 ticks_per_s = 10000000;

void mgp_init(void) {
    LARGE_INTEGER perf_freq = { 0 };
    if (QueryPerformanceFrequency(&perf_freq)) {
        ticks_per_s = ((mgp_u64)perf_freq.HighPart << 32) | perf_freq.LowPart;
    } else {
        // TODO: Error stuff
    }
    timeBeginPeriod(1);
}

const static mgp_u64 _mgp_units_per_sec[MGP_TIMEUNIT_COUNT] = {
    1, 1e3, 1e6, 1e9
};

#define _MGP_WIN_TIMEFUNC(units_per_sec) \
    LARGE_INTEGER perf_count = { 0 }; \
    QueryPerformanceCounter(&perf_count); \
    mgp_u64 ticks = ((mgp_u64)perf_count.HighPart << 32) | perf_count.LowPart; \
    return ticks * units_per_sec / ticks_per_s;

mgp_u64 mgp_gettime_s(void) {
    _MGP_WIN_TIMEFUNC(_mgp_units_per_sec[MGP_SEC]);
}
mgp_u64 mgp_gettime_ms(void) {
    _MGP_WIN_TIMEFUNC(_mgp_units_per_sec[MGP_MILLI_SEC]);
}
mgp_u64 mgp_gettime_us(void) {
    _MGP_WIN_TIMEFUNC(_mgp_units_per_sec[MGP_MICRO_SEC]);
}
mgp_u64 mgp_gettime_ns(void) {
    _MGP_WIN_TIMEFUNC(_mgp_units_per_sec[MGP_NANO_SEC]);
}
mgp_u64 mgp_gettime(mgp_time_unit unit) {
    _MGP_WIN_TIMEFUNC(_mgp_units_per_sec[unit]);
}

#endif // MGP_PLATFORM_WIN32


#ifdef __cplusplus
}
#endif

#endif // MG_PROFILE_IMPL


/*
License
=================================
  _    ___ ___ ___ _  _ ___ ___ 
 | |  |_ _/ __| __| \| / __| __|
 | |__ | | (__| _|| .` \__ \ _| 
 |____|___\___|___|_|\_|___/___|
                                
=================================

MIT License

Copyright (c) 2023 Magicalbat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
