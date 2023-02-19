# Magic Arena
## Project Status: Personal Education Project

An [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) library for creating [memory arenas](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).

### Inspired By
- [STB Libraries](https://github.com/nothings/stb)
- [Sokol Libraries](https://github.com/floooh/sokol)
- [Metadesk](https://github.com/Dion-Systems/metadesk)

**Many parts of the implementation are based on the implementation found in Metadesk**

First off, if you are not already familiar with memory arenas, I recommend reading [this](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator) article to know what they are and why they are useful.

**TL;DR**: Arenas are strictly linear allocators that can be faster and easier to work with than the traditional `malloc` and `free` design pattern.

## Documentation

- [Example](#example)
- [Introduction](#introduction)
- [Typedefs](#typedefs)
- [Enums](#enums)
- [Macros](#macros)
- [Structs](#structs)
- [Functions](#functions)
- [Definitions and Options](#definitions-and-options)
- [Error Handling](#error-handling)
- [Platforms](#platforms)

Backends
--------

`mg_arena` uses two different backends depending on the requirements of the application. There is a backend that uses `malloc` and `free`, and there is a backend that uses lower level functions like `VirtualAlloc` and `mmap`.

**NOTE: I recomend using the lower level one, unless you have a good reason not to.**

Example
-------
```c
#include <stdio.h>

// You should put the implementation in a separate source file in a real project
#define MG_ARENA_IMPL
#include "mg_arena.h"

void arena_error(mga_error err) {
    fprintf(stderr, "MGA Error %d: %s\n", err.code, err.msg);
}

int main() {
    mg_arena* arena = mga_create(&(mga_desc){
        .desired_max_size = MGA_MiB(4),
        .desired_block_size = MGA_KiB(256),
        .error_callback = arena_error
    });

    int* data = (int*)mga_push(arena, sizeof(int) * 64);
    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    printf("[ ");
    for (int i = 0; i < 64; i++) {
        printf("%d, ", data[i]);
    }
    printf("\b\b ]\n");

    mga_destroy(arena);

    return 0;
}
```

Introduction
------------

Download the file `mg_arena.h`. Create a source file for the implementation. Add the following:
```c
#define MG_ARENA_IMPL
#include "mg_arena.h"
```

Create an arena by calling `mga_create`, which takes a pointer to a `mga_desc` structure:
```c
mg_arena* arena = mga_create(&(mga_desc){
    .desired_max_size = MGA_MiB(16),
    .desired_block_size = MGA_KiB(256)
});
```
You are required to fill in `desired_max_size`, but all other values will be given defaults by `mga_create`.

Allocate memory by calling `mga_push`:
```c
some_obj* obj = (some_obj*)mga_push(arena, sizeof(some_obj));
```
You can also call `mga_push_zero` to initialize the memory to zero. There are also four macros to make object creation easier:
```c
some_obj* zeroed_obj = (some_obj*)mga_push_zero(arena, sizeof(some_obj));

other_obj* other = MGA_PUSH_STRUCT(arena, other_obj);
other_obj* zeroed_other = MGA_PUSH_ZERO_STRUCT(arena, other_obj);
int* array = MGA_PUSH_ARRAY(arena, int, 64);
int* zeroed_array = MGA_PUSH_ZERO_ARRAY(arena, int, 64);
```

Deallocate memory with `mga_pop` or `mga_pop_to`: 
```c
mga_u64 start_pos = mga_get_pos(arena);
int* arr = MGA_PUSH_ARRAY(arena, int, 64);
// Do something with arr, without allocating any more memory
mga_pop_to(start_pos);
```
**WARNING: Because of memory alignment, `mga_pop` might not deallocate all the memory that you intended it to. It is better to use `mga_pop_to` or temporary arenas (see below).**

Make temporary arenas:
```c
mga_temp temp = mga_temp_begin(arena);

int* data = (int*)mga_push(temp.arena, sizeof(int) * 16);

mga_temp_end(temp);
// data gets deallocated
```

Get temporary memory with scratch arenas:
```c
mga_temp scratch = mga_scratch_get(NULL, 0);

int* data = (int*)mga_push(scratch.arena, sizeof(int) * 64);

mga_scratch_release(scratch);
```

Reset/clear arenas with `mga_reset`:
```c
char* str = (char*)mga_push(arena, sizeof(char) * 10);

mga_reset(arena);
// str gets deallocated
```

Delete arenas with `mga_destroy`:
```c
mg_arena* arena = mga_create(&(mga_desc){
    .desired_max_size = MGA_MiB(16),
    .desired_block_size = MGA_KiB(256)
});

// Do stuff with arena

mga_destroy(arena);
```


Typedefs
--------
- `mga_i8`
    - 8 bit signed integer
- `mga_i16`
    - 16 bit signed integer
- `mga_i32`
    - 32 bit signed integer
- `mga_i64`
    - 64 bit signed integer
- `mga_u8`
    - 8 bit unsigned integer
- `mga_u16`
    - 16 bit unsigned integer
- `mga_u32`
    - 32 bit unsigned integer
- `mga_u64`
    - 64 bit unsigned integer
- `mga_b32`
    - 32 bit boolean
- `mga_error_callback(mga_error error)`
    - Callback function type for errors

Enums
-----
- `mga_error_code`
    - MGA_ERR_NONE
        - No error
    - MGA_ERR_INIT_FAILED
        - Arena failed to init
    - MGA_ERR_MALLOC_FAILED
        - Arena failed to allocate memory with malloc
    - MGA_ERR_COMMIT_FAILED
        - Arena failed to commit memory
    - MGA_ERR_OUT_OF_MEMORY
        - Arena position exceeded arena size
    - MGA_ERR_CANNOT_POP_MORE
        - Arena cannot deallocate any more memory

Macros
------
- `MGA_KiB(x)`
    - Number of bytes per `x` kibibytes (1,024)
- `MGA_MiB(x)`
    - Number of bytes per `x` mebibytes (1,048,576)
- `MGA_GiB(x)`
    - Number of bytes per `x` gibibytes (1,073,741,824)
    
- `MGA_PUSH_STRUCT(arena, type)`
    - Pushes a struct `type` onto `arena`
- `MGA_PUSH_ZERO_STRUCT(arena, type)`
    - Pushes a struct `type` onto `arena` and zeros the memory
- `MGA_PUSH_ARRAY(arena, type, num)`
    - Pushes `num` `type` structs onto `arena`
- `MGA_PUSH_ZERO_ARRAY(arena, type, num)`
    - Pushes `num` `type` structs onto `arena` and zeros the memory

Structs
-------
- `mg_arena` - A memory arena
    - `mga_error_callback*` *error_callback*
        - Error callback function (See `mga_error_callback` for more detail)
    - *(all other properties should only be accessed through the getter functions below)*
- `mga_error` - An error
    - `mga_error_code` *code*
        - Error code (see `mga_error_code` for more detail)
    - `char*` *msg*
        - Error message as a c string
- `mga_desc` - initialization parameters for `mga_create`
    - This struct should be made with designated initializer. All uninitialized values (except for *desired_max_size*) will be given defaults.
    - `mga_u64` *desired_max_size*
        - Maximum size of arena, rounded up to nearest page size
    - `mga_u32` *desired_block_size*
        - Desired size of block in arena. The real block size will be rounded to the nearest page size, then reounded to the nearest power of 2. For the malloc backend, a node will be a multiple of the block size. For the lower level backend, memory is committed in multiples of the block size. (See [Backends](#backends))
    - `mga_u32` *align*
        - Size of memory alignment (See [this article](https://developer.ibm.com/articles/pa-dalign/) for rationality) to apply, **Must be power of 2**. To disable alignment, you can pass in a value of 1.
    - `mga_error_callback*` *error_callback*
        - Error callback function (See `mga_error_callback` for more detail)
- `mga_temp` - A temporary arena
    - `mg_arena*` arena
        - The `mg_arena` object assosiated with the temporary arena


Functions
---------
- `mg_arena* mga_create(const mga_desc* desc)` <br>
    - Creates a new `mg_arena` according to the mga_desc object.
    - Returns NULL on failure, get the error with the callback function or with `mga_get_error`
- `void mga_destroy(mg_arena* arena)` <br>
    - Destroys an `mg_arena` object.
- `mga_error mga_get_error(mg_arena* arena)` <br>
    - Gets the last error from the given arena. **Arena can be NULL.** If the arena is null, it will give the last error according to a static, thread local variable in the implementation.
- `mga_u64 mga_get_pos(mg_arena* arena)`
- `mga_u64 mga_get_size(mg_arena* arena)`
- `mga_u32 mga_get_block_size(mg_arena* arena)`
- `mga_u32 mga_get_align(mg_arena* arena)`
    - (See `mga_desc` for more detail about what these mean)
- `void* mga_push(mg_arena* arena, mga_u64 size)`
    - Allocates `size` bytes on the arena.
    - Retruns NULL on failure
- `void* mga_push_zero(mg_arena* arena, mga_u64 size)`
    - Allocates `size` bytes on the arena and zeros the memory.
    - Returns NULL on failure
- `void mga_pop(mg_arena* arena, mga_u64 size)`
    - Pops `size` bytes from the arena.
    - **WARNING: Because of memory alignment, this may not always act as expected. Make sure you know what you are doing.**
    - Fails if you attempt to pop too much memory
- `void mga_pop_to(mg_arena* arena, mga_u64 pos)`
    - Pops memory from the arena, setting the arenas position to `pos`.
    - **WARNING: Because of memory alignment, this may not always act as expected. Make sure you know what you are doing.**
    - Fails if you attempt to pop too much memory
- `void mga_reset(mg_arena* arena)`
    - Deallocates all memory in arena, returning the arena to its original position.
    - NOTE: Always use `mga_reset` instead of `mga_pop_to` if you need to clear all memory. Position 0 is not always the start of the arena. 
- `mga_temp mga_temp_begin(mg_arena* arena)`
    - Creates a new temporary arena from the given arena.
- `void mga_temp_end(mga_temp temp)`
    - Destroys the temporary arena, deallocating all allocations made with the temporary arena.
- `void mga_scratch_set_desc(mga_desc* desc)`
    - Sets the `mga_desc` used to initialize scratch arenas.
    - NOTE: This will only work before any calls to `mga_scratch_get`
    - The default desc has a `desired_max_size` of 64 MiB and a `desired_block_size` of 128 KiB
- `mga_temp mga_scratch_get(mg_arena** conflicts, mga_u32 num_conflicts)`
    - Gets a thread local scratch arena
    - You can pass in a list of conflict scratch arenas. One example where this is useful is if you have a function that gets a scratch arena calling another function that gets another scratch arena:
        - ```c
          int* func_b(mg_arena* arena) {
              mga_temp scratch = mga_scratch_get(&arena, 1);
              // Do stuff
              mga_scratch_release(scratch);
          }
          void func_a() {
              mga_temp scratch = mga_scratch_get(NULL, 0);
              func_b(scratch);
              mga_scratch_release(scratch);
          }
- `void mga_scratch_release(mga_temp scratch)`
    - Releases the scratch arena

Definitions and Options
-----------------------

Define these above where you put the implementation. Example:
```c
#define MGA_FORCE_MALLOC
#define MGA_MALLOC custom_malloc
#define MGA_FREE custom_free
#define MG_ARENA_IMPL
#include "mg_arena.h"
```

- `MGA_FORCE_MALLOC`
    - Enables the `malloc` based backend
- `MGA_MALLOC` and `MGA_FREE`
    - If you are using the malloc backend (because of an unknown platform or `MGA_FORCE_MALLOC`), you can provide your own implementations of `malloc` and `free` to avoid the c standard library.
- `MGA_MEMSET`
    - Provide a custom implementation of `memset` to avoid the c standard library.
- `MGA_THREAD_VAR`
    - Provide the implementation for creating a thread local variable if it is not supported.
- `MGA_FUNC_DEF`
    - Add custom prefix to all functions
- `MGA_STATIC`
    - Makes all functions static.
- `MGA_DLL`
    - Adds `__declspec(dllexport)` or `__declspec(dllimport)` to all functions.
    - NOTE: `MGA_STATIC` and `MGA_DLL` do not work simultaneously and they do not work if you have defined `MGA_FNC_DEF`.
- `MGA_SCRATCH_COUNT`
    - Number of scratch arenas per thread
    - Default is 2
- `MGA_MEM_RESERVE` and related
    - See [Platforms](#platforms)

Error Handling
--------------
There are two ways to do error handling, you can use both or neither. **Errors will not be displayed by default.** <br>
An error has a code (`mga_error_code` enum) and a c string (`char*`) message;

- Callback functions
    - The first way is to register a callback function. The callback function is unique to the arena, so you can mix and match if you like.
     ```c
    #include <stdio.h>

    // In a real application, the implementaion should be in another c file
    #define MG_ARENA_IMPL
    #include "mg_arena.h"

    void error_callback(mga_error error) {
        fprintf(stderr, "MGA Error %d: %s\n", error.code, error.msg);
    }

    int main() {
        mg_arena* arena = mga_create(&(mga_desc){
            .desired_max_size = MGA_MiB(4),
            .error_callback = error_callback
        });

        mga_push(arena, MGA_MiB(5));
        // error_callback gets called

        mga_destroy(arena);

        return 0;
    }
     ```
- `mga_error mga_get_error(mg_arena* arena)`
    - This is another way to get the error struct.
    - This function does work with a NULL pointer in case the arena is not initialized correctly. If the pointer given to the function is NULL, the function uses a thead local and static variable.
    - I would recommend using the callback because it will always be assosiated with the arena.
    ```c
    #include <stdio.h>

    // In a real application, the implementaion should be in another c file
    #define MG_ARENA_IMPL
    #include "mg_arena.h"

    int main() {
        mg_arena* arena = mga_create(&(mga_desc){
            .desired_max_size = MGA_MiB(4),
        });

        int* data = (int*)mga_push(arena, MGA_MiB(5));
        if (data == NULL) {
            mga_error error = mga_get_error(arena);
            fprintf(stderr, "MGA Error %d: %s\n", error.code, error.msg);
        }

        mga_destroy(arena);

        return 0;
    }
    ```

Platforms
---------
Here is a list of the platforms that are currently supported:
- Windows
- Linux
- MacOS
- Emscripten

Using the low level backend is always prefered by mg_arena, but the malloc backend is used if it is not possible or the platform is unknown. If you are using a platform the is unsupported **and** do not want to use the malloc backend, you can do the following:

(For this example, I will show how you could implement this for Windows, even though you would not actually have to do this for Windows).

To use the low level backend for an unknown platform, you have to create five functions and set corresponding definitions. I would recommend using `<stdint.h>` or something similar for these functions.
```c
// Reserves size bytes
// Returns pointer to data
void* mem_reserve(uint64_t size);
// Commits size bytes, starting at ptr
// Returns 1 if the commit worked, 0 on failure
int32_t mem_commit(void* ptr, uint64_t size);
// Decommits size bytes, starting at ptr
void mem_decommit(void* ptr, uint64_t size);
// Releases size bytes, starting at ptr
void mem_release(void* ptr, uint64_t size);
// Gets the page size of the system
uint32_t mem_pagesize();

#define MGA_MEM_RESERVE mem_reserve
#define MGA_MEM_COMMIT mem_commit
#define MGA_MEM_DECOMMIT mem_decommit
#define MGA_MEM_RELEASE mem_release
#define MGA_MEM_PAGESIZE mem_pagesize

#define MG_ARENA_IMPL
#include "mg_arena.h"
```

Here is what it would look like on Windows:
```c
#include <stdint.h>
#include <Windows.h>

void* win32_mem_reserve(uint64_t size) {
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}
int32_t win32_mem_commit(void* ptr, uint64_t size) {
    return (int32_t)(VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
}
void win32_mem_decommit(void* ptr, uint64_t size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}
void win32_mem_release(void* ptr, uint64_t size) {
    // size is unused
    (void)size;
    VirtualFree(ptr, 0, MEM_RELEASE);
}
uint32_t win32_mem_pagesize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (uint32_t)si.dwPageSize;
}

#define MGA_MEM_RESERVE win32_mem_reserve
#define MGA_MEM_COMMIT win32_mem_commit
#define MGA_MEM_DECOMMIT win32_mem_decommit
#define MGA_MEM_RELEASE win32_mem_release
#define MGA_MEM_PAGESIZE win32_mem_pagesize

#define MG_ARENA_IMPL
#include "mg_arena.h"
```

### TODO
- Testing
- Article about implementation
