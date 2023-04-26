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
// TODO: option string.h
#include <string.h>
#include <assert.h>

typedef int8_t   mgp_i8;
typedef int16_t  mgp_i16;
typedef int32_t  mgp_i32;
typedef int64_t  mgp_i64;
typedef uint8_t  mgp_u8;
typedef uint16_t mgp_u16;
typedef uint32_t mgp_u32;
typedef uint64_t mgp_u64;

typedef mgp_i32 mgp_b32;

typedef float mgp_f32;
typedef double mgp_f64;

static_assert(sizeof(mgp_f32) == 4, "MGP Required 4 bytes floats");
static_assert(sizeof(mgp_f64) == 8, "MGP Required 8 bytes doubles");

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

typedef struct {
    mgp_f64 total_time;
    mgp_f64 average_time;
} mgp_info;

#define MGP_MAX_MULTI 4096
typedef struct {
    mgp_u32 size;
    mgp_f64 total_times[MGP_MAX_MULTI];
    mgp_f64 average_times[MGP_MAX_MULTI];
} mgp_multi_info;

typedef void(mgp_basic_func)(void*);

MGP_FUNC_DEF mgp_info mgp_profile_basic(mgp_basic_func* func, void* func_arg, mgp_u64 iters, mgp_time_unit unit);
MGP_FUNC_DEF void mgp_profile_multi_basic(mgp_basic_func* func, void* func_arg, mgp_u64 iters, mgp_u32 per_iter, mgp_time_unit unit, mgp_multi_info* out);

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

static const mgp_u64 _mgp_sec_mul[MGP_TIMEUNIT_COUNT] = {
    1, 1e3, 1e6, 1e9
};
static const mgp_u64 _mgp_nsec_div[MGP_TIMEUNIT_COUNT] = {
    1e9, 1e6, 1e3, 1
};

#if defined(MGP_PLATFORM_LINUX) || defined(MGP_PLATFORM_APPLE)

#include <time.h>

#ifdef MGP_PLATFORM_APPLE
#    include <mach/clock.h>
#    include <mach/mach.h>
#endif

// https://gist.github.com/jbenet/1087739

void mgp_init(void) { }

#if defined(MGP_PLATFORM_LINUX)
#    define _MGP_UNIX_GET_TS(ts) do { \
        clock_gettime(CLOCK_MONOTONIC, &ts); \
    } while (0)
#elif defined(MGP_PLATFORM_APPLE)
#    define _MGP_UNIX_GET_TS(ts) do { \
        clock_serv_t cclock; \
        mach_timespec_t mts; \
        host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &cclock); \
        clock_get_time(cclock, &mts); \
        mach_port_deallocate(mach_task_self(), cclock); \
        ts.tv_sec = mts.tv_sec; \
        ts.tv_nsec = mts.tv_nsec; \
    } while (0)
#endif

mgp_u64 mgp_gettime_s(void) { 
    struct timespec ts = { 0 };
    _MGP_UNIX_GET_TS(ts);
    return ts.tv_sec * _mgp_sec_mul[MGP_SEC] + ts.tv_nsec / _mgp_nsec_div[MGP_SEC];
}
mgp_u64 mgp_gettime_ms(void) { 
    struct timespec ts = { 0 };
    _MGP_UNIX_GET_TS(ts);
    return ts.tv_sec * _mgp_sec_mul[MGP_MILLI_SEC] + ts.tv_nsec / _mgp_nsec_div[MGP_MILLI_SEC];
}
mgp_u64 mgp_gettime_us(void) { 
    struct timespec ts = { 0 };
    _MGP_UNIX_GET_TS(ts);
    return ts.tv_sec * _mgp_sec_mul[MGP_MICRO_SEC] + ts.tv_nsec / _mgp_nsec_div[MGP_MICRO_SEC];
}
mgp_u64 mgp_gettime_ns(void) { 
    struct timespec ts = { 0 };
    _MGP_UNIX_GET_TS(ts);
    return ts.tv_sec * _mgp_sec_mul[MGP_NANO_SEC] + ts.tv_nsec / _mgp_nsec_div[MGP_NANO_SEC];
}
mgp_u64 mgp_gettime(mgp_time_unit unit) {
    if (unit < 0 || unit >= MGP_TIMEUNIT_COUNT)
        return 0;
    
    struct timespec ts = { 0 };
    _MGP_UNIX_GET_TS(ts);
    return ts.tv_sec * _mgp_sec_mul[unit] + ts.tv_nsec / _mgp_nsec_div[unit];
}

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


#define _MGP_WIN_TIMEFUNC(units_per_sec) \
    LARGE_INTEGER perf_count = { 0 }; \
    QueryPerformanceCounter(&perf_count); \
    mgp_u64 ticks = ((mgp_u64)perf_count.HighPart << 32) | perf_count.LowPart; \
    return ticks * units_per_sec / ticks_per_s;

mgp_u64 mgp_gettime_s(void) {
    _MGP_WIN_TIMEFUNC(_mgp_sec_mul[MGP_SEC]);
}
mgp_u64 mgp_gettime_ms(void) {
    _MGP_WIN_TIMEFUNC(_mgp_sec_mul[MGP_MILLI_SEC]);
}
mgp_u64 mgp_gettime_us(void) {
    _MGP_WIN_TIMEFUNC(_mgp_sec_mul[MGP_MICRO_SEC]);
}
mgp_u64 mgp_gettime_ns(void) {
    _MGP_WIN_TIMEFUNC(_mgp_sec_mul[MGP_NANO_SEC]);
}
mgp_u64 mgp_gettime(mgp_time_unit unit) {
    if (unit < 0 || unit >= MGP_TIMEUNIT_COUNT)
        return 0;
    _MGP_WIN_TIMEFUNC(_mgp_sec_mul[unit]);
}

#endif // MGP_PLATFORM_WIN32

mgp_info mgp_profile_basic(mgp_basic_func* func, void* func_arg, mgp_u64 iters, mgp_time_unit unit) {
    mgp_info out = { 0 };

    if (iters == 0)
        return out;

    for (mgp_u64 i = 0; i < iters; i++) {
        mgp_u64 start = mgp_gettime(unit);
        
        func(func_arg);
        
        mgp_u64 end = mgp_gettime(unit);

        out.total_time += (mgp_f64)(end - start);
    }

    out.average_time = out.total_time / iters;

    return out;
}

void mgp_profile_multi_basic(mgp_basic_func* func, void* func_arg, mgp_u64 iters, mgp_u32 per_iter, mgp_time_unit unit, mgp_multi_info* out) {
    if (iters > MGP_MAX_MULTI || per_iter == 0) {
        return;
    }
    
    out->size = iters;

    for (mgp_u64 i = 0; i < iters; i++) {
        out->total_times[i] = 0;

        for (mgp_u64 j = 0; j < per_iter; j++) {
            mgp_u64 start = mgp_gettime(unit);

            func(func_arg);

            mgp_u64 end = mgp_gettime(unit);
            
            out->total_times[i] += (mgp_f64)(end - start);
        }
        
        out->average_times[i] = out->total_times[i] / per_iter;
    }
}

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
