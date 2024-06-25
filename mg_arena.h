/*
MGA Header
===================================================
  __  __  ___   _     _  _ ___   _   ___  ___ ___ 
 |  \/  |/ __| /_\   | || | __| /_\ |   \| __| _ \
 | |\/| | (_ |/ _ \  | __ | _| / _ \| |) | _||   /
 |_|  |_|\___/_/ \_\ |_||_|___/_/ \_\___/|___|_|_\

===================================================
*/

#ifndef MG_ARENA_H
#define MG_ARENA_H

#ifndef MGA_FUNC_DEF
#   if defined(MGA_STATIC)
#      define MGA_FUNC_DEF static
#   elif defined(_WIN32) && defined(MGA_DLL) && defined(MG_ARENA_IMPL)
#       define MGA_FUNC_DEF __declspec(dllexport)
#   elif defined(_WIN32) && defined(MGA_DLL)
#       define MGA_FUNC_DEF __declspec(dllimport)
#   else
#      define MGA_FUNC_DEF extern
#   endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int32_t  mga_i32;
typedef uint8_t  mga_u8;
typedef uint32_t mga_u32;
typedef uint64_t mga_u64;

typedef mga_i32 mga_b32;

#define MGA_KiB(x) (mga_u64)((mga_u64)(x) << 10)
#define MGA_MiB(x) (mga_u64)((mga_u64)(x) << 20)
#define MGA_GiB(x) (mga_u64)((mga_u64)(x) << 30) 

typedef struct _mga_malloc_node {
    struct _mga_malloc_node* prev;
    mga_u64 size;
    mga_u64 pos;
    mga_u8* data;
} _mga_malloc_node;

typedef struct {
    _mga_malloc_node* cur_node;
} _mga_malloc_backend;
typedef struct {
    mga_u64 commit_pos;
} _mga_reserve_backend;

typedef enum {
    MGA_ERR_NONE = 0,
    MGA_ERR_INIT_FAILED,
    MGA_ERR_MALLOC_FAILED,
    MGA_ERR_COMMIT_FAILED,
    MGA_ERR_OUT_OF_MEMORY,
    MGA_ERR_CANNOT_POP_MORE
} mga_error_code;

typedef struct {
    mga_error_code code;
    char* msg;
} mga_error;

typedef void (mga_error_callback)(mga_error error);


typedef struct {
    mga_u64 _pos;

    mga_u64 _size;
    mga_u64 _block_size;
    mga_u32 _align;

    union {
        _mga_malloc_backend _malloc_backend;
        _mga_reserve_backend _reserve_backend;
    };

    mga_error _last_error;
    mga_error_callback* error_callback;
} mg_arena;

typedef struct {
    mga_u64 desired_max_size;
    mga_u32 desired_block_size;
    mga_u32 align;
    mga_error_callback* error_callback;
} mga_desc;

MGA_FUNC_DEF mg_arena* mga_create(const mga_desc* desc);
MGA_FUNC_DEF void mga_destroy(mg_arena* arena);

MGA_FUNC_DEF mga_error mga_get_error(mg_arena* arena);

MGA_FUNC_DEF mga_u64 mga_get_pos(mg_arena* arena);
MGA_FUNC_DEF mga_u64 mga_get_size(mg_arena* arena);
MGA_FUNC_DEF mga_u32 mga_get_block_size(mg_arena* arena);
MGA_FUNC_DEF mga_u32 mga_get_align(mg_arena* arena);

MGA_FUNC_DEF void* mga_push(mg_arena* arena, mga_u64 size);
MGA_FUNC_DEF void* mga_push_zero(mg_arena* arena, mga_u64 size);

MGA_FUNC_DEF void mga_pop(mg_arena* arena, mga_u64 size);
MGA_FUNC_DEF void mga_pop_to(mg_arena* arena, mga_u64 pos);

MGA_FUNC_DEF void mga_reset(mg_arena* arena);

#define MGA_PUSH_STRUCT(arena, type) (type*)mga_push(arena, sizeof(type))
#define MGA_PUSH_ZERO_STRUCT(arena, type) (type*)mga_push_zero(arena, sizeof(type))
#define MGA_PUSH_ARRAY(arena, type, num) (type*)mga_push(arena, sizeof(type) * (num))
#define MGA_PUSH_ZERO_ARRAY(arena, type, num) (type*)mga_push_zero(arena, sizeof(type) * (num))

typedef struct {
    mg_arena* arena;
    mga_u64 _pos;
} mga_temp;

MGA_FUNC_DEF mga_temp mga_temp_begin(mg_arena* arena);
MGA_FUNC_DEF void mga_temp_end(mga_temp temp);

MGA_FUNC_DEF void mga_scratch_set_desc(const mga_desc* desc);
MGA_FUNC_DEF mga_temp mga_scratch_get(mg_arena** conflicts, mga_u32 num_conflicts);
MGA_FUNC_DEF void mga_scratch_release(mga_temp scratch);

#ifdef __cplusplus
}
#endif

#endif // MG_ARENA_H

/*
MGA Implementation
===========================================================================================
  __  __  ___   _     ___ __  __ ___ _    ___ __  __ ___ _  _ _____ _ _____ ___ ___  _  _ 
 |  \/  |/ __| /_\   |_ _|  \/  | _ \ |  | __|  \/  | __| \| |_   _/_\_   _|_ _/ _ \| \| |
 | |\/| | (_ |/ _ \   | || |\/| |  _/ |__| _|| |\/| | _|| .` | | |/ _ \| |  | | (_) | .` |
 |_|  |_|\___/_/ \_\ |___|_|  |_|_| |____|___|_|  |_|___|_|\_| |_/_/ \_\_| |___\___/|_|\_|

===========================================================================================
*/

#ifdef MG_ARENA_IMPL

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__)
#    define MGA_PLATFORM_LINUX
#elif defined(__APPLE__)
#    define MGA_PLATFORM_APPLE
#elif defined(_WIN32)
#    define MGA_PLATFORM_WIN32
#elif defined(__EMSCRIPTEN__)
#    define MGA_PLATFORM_EMSCRIPTEN
#else
#    warning "MGA: Unknown platform"
#    define MGA_PLATFORM_UNKNOWN
#endif

#if defined(MGA_MEM_RESERVE) && defined(MGA_MEM_COMMIT) && defined(MGA_MEM_DECOMMIT) && defined(MGA_MEM_RELEASE) && defined(MGA_MEM_PAGESIZE)
#elif !defined(MGA_MEM_RESERVE) && !defined(MGA_MEM_COMMIT) && !defined(MGA_MEM_DECOMMIT) && !defined(MGA_MEM_RELEASE) && !defined(MGA_MEM_PAGESIZE)
#else
#    error "MG ARENA: Must define all or none of, MGA_MEM_RESERVE, MGA_MEM_COMMIT, MGA_MEM_DECOMMIT, MGA_MEM_RELEASE, and MGA_MEM_PAGESIZE"
#endif

#if !defined(MGA_MEM_RESERVE) && !defined(MGA_FORCE_MALLOC) && (defined(MGA_PLATFORM_LINUX) || defined(MGA_PLATFORM_WIN32))
#    define MGA_MEM_RESERVE _mga_mem_reserve
#    define MGA_MEM_COMMIT _mga_mem_commit
#    define MGA_MEM_DECOMMIT _mga_mem_decommit
#    define MGA_MEM_RELEASE _mga_mem_release
#    define MGA_MEM_PAGESIZE _mga_mem_pagesize
#endif

// This is needed for the size and block_size calculations
#ifndef MGA_MEM_PAGESIZE
#    define MGA_MEM_PAGESIZE _mga_mem_pagesize
#endif

#if !defined(MGA_MEM_RESERVE) && !defined(MGA_FORCE_MALLOC)
#   define MGA_FORCE_MALLOC
#endif

#if defined(MGA_FORCE_MALLOC)
#    if defined(MGA_MALLOC) && defined(MGA_FREE)
#    elif !defined(MGA_MALLOC) && !defined(MGA_FREE)
#    else
#        error "MGA ARENA: Must define both or none of MGA_MALLOC and MGA_FREE"
#    endif
#    ifndef MGA_MALLOC
#        include <stdlib.h>
#        define MGA_MALLOC malloc
#        define MGA_FREE free
#    endif
#endif

#ifndef MGA_MEMSET
#   include <string.h>
#   define MGA_MEMSET memset
#endif

#ifndef MGA_NO_STDIO
#   include <stdio.h>
#endif

#define MGA_UNUSED(x) (void)(x)

#define MGA_TRUE 1
#define MGA_FALSE 0

#ifndef MGA_THREAD_VAR
#    if defined(__clang__) || defined(__GNUC__)
#        define MGA_THREAD_VAR __thread
#    elif defined(_MSC_VER)
#        define MGA_THREAD_VAR __declspec(thread)
#    elif (__STDC_VERSION__ >= 201112L)
#        define MGA_THREAD_VAR _Thread_local
#    else
#        error "MG ARENA: Invalid compiler/version for thead variable; Define MGA_THREAD_VAR, use Clang, GCC, or MSVC, or use C11 or greater"
#    endif
#endif

#define MGA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MGA_MAX(a, b) ((a) > (b) ? (a) : (b))

#define MGA_ALIGN_UP_POW2(x, b) (((x) + ((b) - 1)) & (~((b) - 1)))

#ifdef MGA_PLATFORM_WIN32

#ifndef UNICODE
    #define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#ifndef MGA_FORCE_MALLOC
static void* _mga_mem_reserve(mga_u64 size) {
    void* out = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return out;
}
static mga_b32 _mga_mem_commit(void* ptr, mga_u64 size) {
    mga_b32 out = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return out;
}
static void _mga_mem_decommit(void* ptr, mga_u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}
static void _mga_mem_release(void* ptr, mga_u64 size) {
    MGA_UNUSED(size);
    VirtualFree(ptr, 0, MEM_RELEASE);
}
#endif
static mga_u32 _mga_mem_pagesize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (mga_u32)si.dwPageSize;
}

#endif // MGA_PLATFORM_WIN32

#if defined(MGA_PLATFORM_LINUX) || defined(MGA_PLATFORM_APPLE)

#include <sys/mman.h>
#include <unistd.h>

#ifndef MGA_FORCE_MALLOC
static void* _mga_mem_reserve(mga_u64 size) {
    void* out = mmap(NULL, size, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, (off_t)0);
    return out;
}
static mga_b32 _mga_mem_commit(void* ptr, mga_u64 size) {
    mga_b32 out = (mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0);
    return out;
}
static void _mga_mem_decommit(void* ptr, mga_u64 size) {
    mprotect(ptr, size, PROT_NONE);
    madvise(ptr, size, MADV_DONTNEED);
}
static void _mga_mem_release(void* ptr, mga_u64 size) {
    munmap(ptr, size);
}
#endif
static mga_u32 _mga_mem_pagesize() {
    return (mga_u32)sysconf(_SC_PAGESIZE);
}

#endif // MGA_PLATFORM_LINUX || MGA_PLATFORM_APPLE

#ifdef MGA_PLATFORM_UNKNOWN

#ifndef MGA_FORCE_MALLOC
static void* _mga_mem_reserve(mga_u64 size) { MGA_UNUSED(size); return NULL; }
static void _mga_mem_commit(void* ptr, mga_u64 size) { MGA_UNUSED(ptr); MGA_UNUSED(size); }
static void _mga_mem_decommit(void* ptr, mga_u64 size) { MGA_UNUSED(ptr); MGA_UNUSED(size); }
static void _mga_mem_release(void* ptr, mga_u64 size) { MGA_UNUSED(ptr); MGA_UNUSED(size); }
#endif
static mga_u32 _mga_mem_pagesize(){ return 4096; }

#endif // MGA_PLATFORM_UNKNOWN

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static mga_u32 _mga_round_pow2(mga_u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    
    return v;
}


typedef struct {
    mga_error_callback* error_callback;
    mga_u64 max_size;
    mga_u32 block_size;
    mga_u32 align;
} _mga_init_data;

static void _mga_empty_error_callback(mga_error error) {
    MGA_UNUSED(error);
}

static _mga_init_data _mga_init_common(const mga_desc* desc) {
    _mga_init_data out = { 0 };
    
    out.error_callback = desc->error_callback == NULL ?
        _mga_empty_error_callback : desc->error_callback;

    mga_u32 page_size = MGA_MEM_PAGESIZE();
    
    out.max_size = MGA_ALIGN_UP_POW2(desc->desired_max_size, page_size);
    mga_u32 desired_block_size = desc->desired_block_size == 0 ? 
        MGA_ALIGN_UP_POW2(out.max_size / 8, page_size) : desc->desired_block_size;
    desired_block_size = MGA_ALIGN_UP_POW2(desired_block_size, page_size);
    
    out.block_size = _mga_round_pow2(desired_block_size);
    
    out.align = desc->align == 0 ? (sizeof(void*)) : desc->align;
    
    return out;
}

// This is an annoying placement, but
// it has to be above the implementations that reference it
static MGA_THREAD_VAR mga_error last_error;

#ifdef MGA_FORCE_MALLOC

/*
Malloc Backend
======================================================================
  __  __   _   _    _    ___   ___   ___   _   ___ _  _____ _  _ ___  
 |  \/  | /_\ | |  | |  / _ \ / __| | _ ) /_\ / __| |/ / __| \| |   \
 | |\/| |/ _ \| |__| |_| (_) | (__  | _ \/ _ \ (__| ' <| _|| .` | |) |
 |_|  |_/_/ \_\____|____\___/ \___| |___/_/ \_\___|_|\_\___|_|\_|___/ 

======================================================================
*/
                                                                      
mg_arena* mga_create(const mga_desc* desc) {
    _mga_init_data init_data = _mga_init_common(desc);

    mg_arena* out = (mg_arena*)malloc(sizeof(mg_arena));

    if (out == NULL) {
        last_error.code = MGA_ERR_INIT_FAILED;
        last_error.msg = "Failed to malloc initial memory for arena";
        init_data.error_callback(last_error);
        return NULL;
    }
    
    out->_pos = 0;
    out->_size = init_data.max_size;
    out->_block_size = init_data.block_size;
    out->_align = init_data.align;
    out->_last_error = (mga_error){ .code=MGA_ERR_NONE, .msg="" };
    out->error_callback = init_data.error_callback;

    out->_malloc_backend.cur_node = (_mga_malloc_node*)malloc(sizeof(_mga_malloc_node));
    *out->_malloc_backend.cur_node = (_mga_malloc_node){
        .prev = NULL,
        .size = out->_block_size,
        .pos = 0,
        .data = (mga_u8*)malloc(out->_block_size)
    };

    return out;
}
void mga_destroy(mg_arena* arena) {
    _mga_malloc_node* node = arena->_malloc_backend.cur_node;
    while (node != NULL) {
        free(node->data);

        _mga_malloc_node* temp = node;
        node = node->prev;
        free(temp);
    }
    
    free(arena);
}

void* mga_push(mg_arena* arena, mga_u64 size) {
    if (arena->_pos + size > arena->_size) {
        last_error.code = MGA_ERR_OUT_OF_MEMORY;
        last_error.msg = "Arena ran out of memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }

    _mga_malloc_node* node = arena->_malloc_backend.cur_node;

    mga_u64 pos_aligned = MGA_ALIGN_UP_POW2(node->pos, arena->_align);
    mga_u32 diff = pos_aligned - node->pos;
    arena->_pos += diff + size;

    if (arena->_pos >= node->size) {
        
        mga_u64 unclamped_node_size = MGA_ALIGN_UP_POW2(size, arena->_block_size);
        mga_u64 max_node_size = arena->_size - arena->_pos;
        mga_u64 node_size = MGA_MIN(unclamped_node_size, max_node_size);
        
        _mga_malloc_node* new_node = (_mga_malloc_node*)malloc(sizeof(_mga_malloc_node));
        mga_u8* data = (mga_u8*)malloc(node_size);

        if (new_node == NULL || data == NULL) {
            if (new_node != NULL) { free(new_node); }
            if (data != NULL) { free(data); }
            
            last_error.code = MGA_ERR_MALLOC_FAILED;
            last_error.msg = "Failed to malloc new node";
            arena->_last_error = last_error;
            arena->error_callback(last_error);
            return NULL;
        }

        new_node->pos = size;
        new_node->size = node_size;
        new_node->data = data;
        
        new_node->prev = node;
        arena->_malloc_backend.cur_node = new_node;

        return (void*)(new_node->data);
    }
    
    void* out = (void*)((mga_u8*)node->data + pos_aligned);
    node->pos = pos_aligned + size;

    return out;
}

void mga_pop(mg_arena* arena, mga_u64 size) {
    if (size > arena->_pos) {
        last_error.code = MGA_ERR_CANNOT_POP_MORE;
        last_error.msg = "Attempted to pop too much memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
    }
    
    mga_u64 size_left = size;
    _mga_malloc_node* node = arena->_malloc_backend.cur_node;

    while (size_left > node->pos) {
        size_left -= node->pos;
        
        _mga_malloc_node* temp = node;
        node = node->prev;

        free(temp->data);
        free(temp);
    }

    arena->_malloc_backend.cur_node = node;

    node->pos -= size_left;
    arena->_pos -= size;
}

void mga_reset(mg_arena* arena) {
    mga_pop_to(arena, 0);
}

#else // MGA_FORCE_MALLOC

/*
Low Level Backend
================================================================================
  _    _____      __  _    _____   _____ _      ___   _   ___ _  _____ _  _ ___  
 | |  / _ \ \    / / | |  | __\ \ / / __| |    | _ ) /_\ / __| |/ / __| \| |   \
 | |_| (_) \ \/\/ /  | |__| _| \ V /| _|| |__  | _ \/ _ \ (__| ' <| _|| .` | |) |
 |____\___/ \_/\_/   |____|___| \_/ |___|____| |___/_/ \_\___|_|\_\___|_|\_|___/ 

================================================================================
*/

#define MGA_MIN_POS MGA_ALIGN_UP_POW2(sizeof(mg_arena), 64) 

mg_arena* mga_create(const mga_desc* desc) {
    _mga_init_data init_data = _mga_init_common(desc);
    
    mg_arena* out = MGA_MEM_RESERVE(init_data.max_size);

    if (out == NULL) {
        last_error.code = MGA_ERR_INIT_FAILED;
        last_error.msg = "Failed to reserve initial memory for arena";
        init_data.error_callback(last_error);
        return NULL;
    }

    if (!MGA_MEM_COMMIT(out, init_data.block_size)) {
        last_error.code = MGA_ERR_INIT_FAILED;
        last_error.msg = "Failed to commit initial memory for arena";
        init_data.error_callback(last_error);
        return NULL;
    }

    out->_pos = MGA_MIN_POS;
    out->_size = init_data.max_size;
    out->_block_size = init_data.block_size;
    out->_align = init_data.align;
    out->_reserve_backend.commit_pos = init_data.block_size;
    out->_last_error = (mga_error){ .code=MGA_ERR_NONE, .msg="" };
    out->error_callback = init_data.error_callback;

    return out;
}
void mga_destroy(mg_arena* arena) {
    MGA_MEM_RELEASE(arena, arena->_size);
}

void* mga_push(mg_arena* arena, mga_u64 size) {
    if (arena->_pos + size > arena->_size) {
        last_error.code = MGA_ERR_OUT_OF_MEMORY;
        last_error.msg = "Arena ran out of memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }

    mga_u64 pos_aligned = MGA_ALIGN_UP_POW2(arena->_pos, arena->_align);
    void* out = (void*)((mga_u8*)arena + pos_aligned);
    arena->_pos = pos_aligned + size;

    mga_u64 commit_pos = arena->_reserve_backend.commit_pos;
    if (arena->_pos > commit_pos) {
        mga_u64 commit_unclamped = MGA_ALIGN_UP_POW2(arena->_pos, arena->_block_size);
        mga_u64 new_commit_pos = MGA_MIN(commit_unclamped, arena->_size);
        mga_u64 commit_size = new_commit_pos - commit_pos;
        
        if (!MGA_MEM_COMMIT((void*)((mga_u8*)arena + commit_pos), commit_size)) {
            last_error.code = MGA_ERR_COMMIT_FAILED;
            last_error.msg = "Failed to commit memory";
            arena->_last_error = last_error;
            arena->error_callback(last_error);
            return NULL;
        }

        arena->_reserve_backend.commit_pos = new_commit_pos;
    }

    return out;
}

void mga_pop(mg_arena* arena, mga_u64 size) {
    if (size > arena->_pos - MGA_MIN_POS) {
        last_error.code = MGA_ERR_CANNOT_POP_MORE;
        last_error.msg = "Attempted to pop too much memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);

        return;
    }

    arena->_pos = MGA_MAX(MGA_MIN_POS, arena->_pos - size);

    mga_u64 new_commit = MGA_MIN(arena->_size, MGA_ALIGN_UP_POW2(arena->_pos, arena->_block_size));
    mga_u64 commit_pos = arena->_reserve_backend.commit_pos;

    if (new_commit < commit_pos) {
        mga_u64 decommit_size = commit_pos - new_commit;
        MGA_MEM_DECOMMIT((void*)((mga_u8*)arena + new_commit), decommit_size);
        arena->_reserve_backend.commit_pos = new_commit;
    }
}

void mga_reset(mg_arena* arena) {
    mga_pop_to(arena, MGA_MIN_POS);
}

#endif // NOT MGA_FORCE_MALLOC

/*
All Backends
=========================================================
    _   _    _      ___   _   ___ _  _____ _  _ ___  ___ 
   /_\ | |  | |    | _ ) /_\ / __| |/ / __| \| |   \/ __|
  / _ \| |__| |__  | _ \/ _ \ (__| ' <| _|| .` | |) \__ \
 /_/ \_\____|____| |___/_/ \_\___|_|\_\___|_|\_|___/|___/

=========================================================
*/


mga_error mga_get_error(mg_arena* arena) {
    mga_error* err = arena == NULL ? &last_error : &arena->_last_error;
    mga_error* temp = err;

    *err = (mga_error){ MGA_ERR_NONE, "" };
    
    return *temp;
}

mga_u64 mga_get_pos(mg_arena* arena) { return arena->_pos; }
mga_u64 mga_get_size(mg_arena* arena) { return arena->_size; }
mga_u32 mga_get_block_size(mg_arena* arena) { return arena->_block_size; }
mga_u32 mga_get_align(mg_arena* arena) { return arena->_align; }

void* mga_push_zero(mg_arena* arena, mga_u64 size) {
    mga_u8* out = mga_push(arena, size);
    MGA_MEMSET(out, 0, size);
    
    return (void*)out;
}

void mga_pop_to(mg_arena* arena, mga_u64 pos) {
    mga_pop(arena, arena->_pos - pos);
}

mga_temp mga_temp_begin(mg_arena* arena) {
    return (mga_temp){
        .arena = arena,
        ._pos = arena->_pos
    };
}
void mga_temp_end(mga_temp temp) {
    mga_pop_to(temp.arena, temp._pos);
}

#ifndef MGA_SCRATCH_COUNT
#   define MGA_SCRATCH_COUNT 2
#endif

#ifndef MGA_NO_STDIO
static void _mga_scratch_on_error(mga_error err) {
    fprintf(stderr, "MGA Scratch Error %u: %s\n", err.code, err.msg);
}
#endif

static MGA_THREAD_VAR mga_desc _mga_scratch_desc = {
    .desired_max_size = MGA_MiB(64),
    .desired_block_size = MGA_KiB(256),
#ifndef MGA_NO_STDIO
    .error_callback = _mga_scratch_on_error,
#endif
};
static MGA_THREAD_VAR mg_arena* _mga_scratch_arenas[MGA_SCRATCH_COUNT] = { 0 };

void mga_scratch_set_desc(const mga_desc* desc) {
    if (_mga_scratch_arenas[0] == NULL) {
        _mga_scratch_desc = (mga_desc){
            .desired_max_size = desc->desired_max_size,
            .desired_block_size = desc->desired_block_size,
            .align = desc->align,
            .error_callback = desc->error_callback
        };
    }
}
mga_temp mga_scratch_get(mg_arena** conflicts, mga_u32 num_conflicts) {
    if (_mga_scratch_arenas[0] == NULL) {
        for (mga_u32 i = 0; i < MGA_SCRATCH_COUNT; i++) {
            _mga_scratch_arenas[i] = mga_create(&_mga_scratch_desc);
        }
    }

    mga_temp out = { 0 };

    for (mga_u32 i = 0; i < MGA_SCRATCH_COUNT; i++) {
        mg_arena* arena = _mga_scratch_arenas[i];

        mga_b32 in_conflict = MGA_FALSE;
        for (mga_u32 j = 0; j < num_conflicts; j++) {
            if (arena == conflicts[j]) {
                in_conflict = MGA_TRUE;
                break;
            }
        }
        if (in_conflict) { continue; }

        out = mga_temp_begin(arena);
    }

    return out;
}
void mga_scratch_release(mga_temp scratch) {
    mga_temp_end(scratch);
}

#ifdef __cplusplus
}
#endif

#endif // MG_ARENA_IMPL

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
