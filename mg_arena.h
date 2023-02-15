/*

TODO: Scratch arenas
TODO: static / extern functions
TODO: dll export functions

*/

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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int8_t   mga_i8;
typedef int16_t  mga_i16;
typedef int32_t  mga_i32;
typedef int64_t  mga_i64;
typedef uint8_t  mga_u8;
typedef uint16_t mga_u16;
typedef uint32_t mga_u32;
typedef uint64_t mga_u64;

typedef mga_i32 mga_b32;

#define MGA_KiB(x) (mga_u64)((mga_u64)(x) << 10)
#define MGA_MiB(x) (mga_u64)((mga_u64)(x) << 20)
#define MGA_GiB(x) (mga_u64)((mga_u64)(x) << 30) 

typedef struct _mga_malloc_node {
    mga_u64 size;
    mga_u64 pos;
    mga_u8* data;
} _mga_malloc_node;

typedef struct {
    _mga_malloc_node* node_arr;
    mga_u32 cur_node;
    mga_u32 num_nodes;
} _mga_malloc_backend;
typedef struct {
    mga_u64 commit_pos;
} _mga_reserve_backend;

typedef enum {
    MGA_ERR_NONE = 0,
    MGA_ERR_INIT_FAILED,
    MGA_ERR_MALLOC_FAILED,
    MGA_ERR_OUT_OF_NODES,
    MGA_ERR_COMMIT_FAILED,
    MGA_ERR_OUT_OF_MEMORY
} mga_error_code;
typedef void (mga_error_callback)(mga_error_code code, char* msg);

typedef struct {
    mga_error_code code;
    char* msg;
} mga_error;

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

mg_arena* mga_create(const mga_desc* desc);
void mga_destroy(mg_arena* arena);

mga_error mga_get_error(mg_arena* arena);

mga_u64 mga_get_pos(mg_arena* arena);
mga_u64 mga_get_size(mg_arena* arena);
mga_u32 mga_get_block_size(mg_arena* arena);
mga_u32 mga_get_align(mg_arena* arena);

void* mga_push(mg_arena* arena, mga_u64 size);
void* mga_push_zero(mg_arena* arena, mga_u64 size);

void mga_pop(mg_arena* arena, mga_u64 size);
void mga_pop_to(mg_arena* arena, mga_u64 pos);

void mga_reset(mg_arena* arena);

#define MGA_PUSH_STRUCT(arena, type) (type*)mga_push(arena, sizeof(type))
#define MGA_PUSH_ZERO_STRUCT(arena, type) (type*)mga_push_zero(arena, sizeof(type))
#define MGA_PUSH_ARRAY(arena, type, num) (type*)mga_push(arena, sizeof(type) * num)
#define MGA_PUSH_ZERO_ARRAY(arena, type, num) (type*)mga_push_zero(arena, sizeof(type) * num)

typedef struct {
    mg_arena* arena;
    mga_u64 _pos;
} mga_temp;

mga_temp mga_temp_begin(mg_arena* arena);
void mga_temp_end(mga_temp temp);

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

#if defined(__linux__) || defined(__APPLE__)
#    define MGA_PLATFORM_LINUX
#elif defined(_WIN32)
#    define MGA_PLATFORM_WIN32
#elif defined(__EMSCRIPTEN__)
#    define MGA_PLATFORM_EMSCRIPTEN
#else
#    warning "MG ARENA: Unknown platform"
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

#if !defined(MGA_MEM_RESERVE)
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

#define MGA_UNUSED(x) (void)(x)

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

void* _mga_mem_reserve(mga_u64 size) {
    void* out = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return out;
}
mga_b32 _mga_mem_commit(void* ptr, mga_u64 size) {
    mga_b32 out = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return out;
}
void _mga_mem_decommit(void* ptr, mga_u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}
void _mga_mem_release(void* ptr, mga_u64 size) {
    MGA_UNUSED(size);
    VirtualFree(ptr, 0, MEM_RELEASE);
}
mga_u32 _mga_mem_pagesize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (mga_u32)si.dwPageSize;
}

#endif // MGA_PLATFORM_WIN32

#ifdef MGA_PLATFORM_LINUX

#include <sys/mman.h>
#include <unistd.h>

void* _mga_mem_reserve(mga_u64 size) {
    void* out = mmap(NULL, size, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, (off_t)0);
    return out;
}
mga_b32 _mga_mem_commit(void* ptr, mga_u64 size) {
    mga_b32 out = (mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0);
    return out;
}
void _mga_mem_decommit(void* ptr, mga_u64 size) {
    mprotect(ptr, size, PROT_NONE);
    madvise(ptr, size, MADV_DONTNEED);
}
void _mga_mem_release(void* ptr, mga_u64 size) {
    munmap(ptr, size);
}
mga_u32 _mga_mem_pagesize() {
    return (mga_u32)sysconf(_SC_PAGESIZE);
}

#endif // MGA_PLATFORM_LINUX

#ifdef MGA_PLATFORM_UNKNOWN

void* _mga_mem_reserve(mga_u64 size) { MGA_UNUSED(size); return NULL; }
void _mga_mem_commit(void* ptr, mga_u64 size) { MGA_UNUSED(ptr); MGA_UNUSED(size); }
void _mga_mem_decommit(void* ptr, mga_u64 size) { MGA_UNUSED(ptr); MGA_UNUSED(size); }
void _mga_mem_release(void* ptr, mga_u64 size) { MGA_UNUSED(ptr); MGA_UNUSED(size); }
mga_u32 _mga_mem_pagesize(){ return 4096; }

#endif // MGA_PLATFORM_UNKNOWN

static void _mga_empty_error_callback(mga_u32 code, char* msg) {
    MGA_UNUSED(code);
    MGA_UNUSED(msg);
}

MGA_THREAD_VAR static mga_error last_error;
mga_error mga_get_error(mg_arena* arena) {
    return arena == NULL ? last_error : arena->_last_error;
}

mga_u64 mga_get_pos(mg_arena* arena) { return arena->_pos; }
mga_u64 mga_get_size(mg_arena* arena) { return arena->_size; }
mga_u32 mga_get_block_size(mg_arena* arena) { return arena->_block_size; }
mga_u32 mga_get_align(mg_arena* arena) { return arena->_align; }

typedef struct {
    mga_error_callback* error_callback;
    mga_u64 max_size;
    mga_u32 block_size;
    mga_u32 align;
} _mga_init_data;

static _mga_init_data _mga_init_common(const mga_desc* desc) {
    _mga_init_data out = { 0 };
    
    out.error_callback = desc->error_callback == NULL ?
        _mga_empty_error_callback : desc->error_callback;

    mga_u32 page_size = MGA_MEM_PAGESIZE();
    
    out.max_size = MGA_ALIGN_UP_POW2(desc->desired_max_size, page_size);
    mga_u32 desired_block_size = desc->desired_block_size == 0 ? 
        MGA_ALIGN_UP_POW2(out.max_size / 8, out.page_size) : desc->desired_block_size;
    out.block_size = MGA_ALIGN_UP_POW2(desired_block_size, page_size);
    
    out.align = desc->align == 0 ? (sizeof(void*)) : desc->align;
    
    return out;
}

void* mga_push_zero(mg_arena* arena, mga_u64 size) {
    void* out = mga_push(arena, size);
    MGA_MEMSET(out, 0, size);
    
    return out;
}

void mga_pop_to(mg_arena* arena, mga_u64 pos) {
    mga_pop(arena, arena->_pos - pos);
}

#ifdef MGA_FORCE_MALLOC

mg_arena* mga_create(const mga_desc* desc) {
    _mga_init_data init_data = _mga_init_common(desc);

    mg_arena* out = (mg_arena*)malloc(sizeof(mg_arena));

    if (out == NULL) {
        last_error.code = MGA_ERR_INIT_FAILED;
        last_error.msg = "Failed to malloc initial memory for arena";
        init_data.error_callback(last_error.code, last_error.msg);
        return NULL;
    }
    
    out->_pos = 0;
    out->_size = init_data.max_size;
    out->_block_size = init_data.block_size;
    out->_align = init_data.align;
    out->error_callback = init_data.error_callback;
    
    mga_u32 num_nodes = (init_data.max_size / init_data.block_size) + 1;
    mga_u64 arr_size = sizeof(_mga_malloc_node) * num_nodes;

    out->_malloc_backend = (_mga_malloc_backend){
        .node_arr = (_mga_malloc_node*)malloc(arr_size),
        .num_nodes = num_nodes,
        .cur_node = 0
    };
    MGA_MEMSET(out->_malloc_backend.node_arr, 0, arr_size);

    _mga_malloc_node* node = &out->_malloc_backend.node_arr[0];
    node->size = out->_block_size;
    node->data = (mga_u8*)malloc(node->size);

    return out;
}
void mga_destroy(mg_arena* arena) {
    for (mga_u32 i = 0; i < arena->_malloc_backend.num_nodes; i++) {
        if (arena->_malloc_backend.node_arr[i].size != 0) {
            free(arena->_malloc_backend.node_arr[i].data);
        }
    }

    free(arena->_malloc_backend.node_arr);
    free(arena);
}

void* mga_push(mg_arena* arena, mga_u64 size) {
    if (arena->_pos + size > arena->_size) {
        last_error.code = MGA_ERR_OUT_OF_MEMORY;
        last_error.msg = "Arena ran out of memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error.code, last_error.msg);
        return NULL;
    }
    
    _mga_malloc_node* node = &arena->_malloc_backend.node_arr[arena->_malloc_backend.cur_node];
    
    mga_u64 pos_aligned = MGA_ALIGN_UP_POW2(node->pos, arena->_align);
    arena->_pos += size;
    
    if (pos_aligned + size >= node->size) {
        mga_u64 node_size = MGA_ALIGN_UP_POW2(size, arena->_block_size);
        arena->_malloc_backend.cur_node += 1;
        
        if (arena->_malloc_backend.cur_node >= arena->_malloc_backend.num_nodes) {
            last_error.code = MGA_ERR_OUT_OF_NODES;
            last_error.msg = "Arena ran out of nodes";
            arena->_last_error = last_error;
            arena->error_callback(last_error.code, last_error.msg);
            
            return NULL;
        }
        node = &arena->_malloc_backend.node_arr[arena->_malloc_backend.cur_node];
        
        node->size = node_size;
        node->pos = size;
        node->data = malloc(node_size);
        
        if (node->data == NULL) {
            last_error.code = MGA_ERR_MALLOC_FAILED;
            last_error.msg = "Failed to malloc node";
            arena->_last_error = last_error;
            arena->error_callback(last_error.code, last_error.msg);

            return NULL;
        }

        return node->data;
    }

    void* out = (void*)((mga_u8*)node->data + pos_aligned);
    node->pos += size;

    return out;
}

void mga_pop(mg_arena* arena, mga_u64 size) {
    if (size > arena->_pos) {
        // TODO: error?
    }
    
    mga_u64 size_left = size;
    mga_u32 node_index = arena->_malloc_backend.cur_node;
    _mga_malloc_node* cur_node = &arena->_malloc_backend.node_arr[arena->_malloc_backend.cur_node];

    arena->_pos -= size;

    while (size_left > cur_node->pos) {
        size_left -= cur_node->pos;

        cur_node->pos = 0;
        cur_node->size = 0;
        free(cur_node->data);

        cur_node -= 1;
        node_index -= 1;

        if (node_index < 0) {
            // TODO: error
        }
    }

    cur_node->pos -= size_left;
    arena->_malloc_backend.cur_node = node_index;
}

void mga_reset(mg_arena* arena) {
    mga_pop_to(arena, 0);
}

#else // MGA_FORCE_MALLOC

#define MGA_MIN_POS MGA_ALIGN_UP_POW2(sizeof(mg_arena), 64) 

mg_arena* mga_create(const mga_desc* desc) {
    _mga_init_data init_data = _mga_init_common(desc);
    
    mg_arena* out = MGA_MEM_RESERVE(init_data.max_size);

    if (!MGA_MEM_COMMIT(out, init_data.block_size)) {
        last_error.code = MGA_ERR_INIT_FAILED;
        last_error.msg = "Failed to commit initial memory for arena";
        init_data.error_callback(last_error.code, last_error.msg);
        return NULL;
    }

    out->_pos = MGA_MIN_POS;
    out->_size = init_data.max_size;
    out->_block_size = init_data.block_size;
    out->_align = init_data.align;
    out->_reserve_backend.commit_pos = init_data.block_size;
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
        arena->error_callback(last_error.code, last_error.msg);
        return NULL;
    }

    mga_u64 pos_aligned = MGA_ALIGN_UP_POW2(arena->_pos, arena->_align);
    void* out = (void*)((mga_u8*)arena + pos_aligned);
    arena->_pos += size;

    mga_u64 commit_pos = arena->_reserve_backend.commit_pos;
    if (arena->_pos > commit_pos) {
        mga_u64 commit_unclamped = MGA_ALIGN_UP_POW2(pos_aligned, arena->_block_size);
        mga_u64 new_commit_pos = MGA_MIN(commit_unclamped, arena->_size);
        mga_u64 commit_size = new_commit_pos - commit_pos;

        if (!MGA_MEM_COMMIT((void*)((mga_u8*)arena + commit_pos), commit_size)) {
            last_error.code = MGA_ERR_COMMIT_FAILED;
            last_error.msg = "Failed to commit memory";
            arena->_last_error = last_error;
            arena->error_callback(last_error.code, last_error.msg);
            return NULL;
        }

        arena->_reserve_backend.commit_pos = new_commit_pos;
    }

    return out;
}

void mga_pop(mg_arena* arena, mga_u64 size) {
    if (size > arena->_size - MGA_MIN_POS) {
        // TODO: error?
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

mga_temp mga_temp_begin(mg_arena* arena) {
    return (mga_temp){
        .arena = arena,
        ._pos = arena->_pos
    };
}
void mga_temp_end(mga_temp temp) {
    mga_pop_to(temp.arena, temp._pos);
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

Copyright (c) 2022 Magicalbat

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