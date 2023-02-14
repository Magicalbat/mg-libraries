# Magic Arena
## Project Status: Personal Education Project

An [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) library for creating [memory arenas](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).

### Inspired By
- [STB Libraries](https://github.com/nothings/stb)
- [Sokol Libraries](https://github.com/floooh/sokol)
- [Metadesk](https://github.com/Dion-Systems/metadesk)

## Quick Start

First off, if you are not already familiar with memory arenas, I recommend reading [this](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator) article to know what they are and why they are useful.

### Tutorial

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
**IMPORTANT: Because of memory alignment, `mga_pop` might not deallocate all the memory that you intended it to. It is better to use `mga_pop_to` or temporary arenas (see below).**

Make temporary arenas:
```c
mga_temp temp = mga_temp_begin(arena);

int* data = (int*)mga_push(temp.arena, sizeof(int) * 16);

mga_temp_end(temp);
// data gets deallocated
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

## Documentation

**NOTE: `mg_arena` uses two different backends depending on the requirements of the application. There is a backend that uses `malloc` and `free`, and there is a backend that uses lower level functions like `VirtualAlloc` and `mmap`.**

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
- `mga_error_callback(mga_error_code code, char* msg)`
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
    - MGA_ERR_OUT_OF_NODES
        - Malloc based arena ran out of memory
    - MGA_ERR_COMMIT_FAILED
        - Arena failed to commit memory
    - MGA_ERR_OUT_OF_MEMORY
        - Arena position exceeded arena size

Macros
------
- MGA_KiB(x)
    - Number of bytes per `x` kibibytes (1024)
- MGA_MiB(x)
    - Number of bytes per `x` mebibytes (1048576)
- MGA_GiB(x)
    - Number of bytes per `x` gibibytes (10737418240)
    
- MGA_PUSH_STRUCT(arena, type)
    - Pushes a struct `type` onto `arena`
- MGA_PUSH_ZERO_STRUCT(arena, type)
    - Pushes a struct `type` onto `arena` and zeros the memory
- MGA_PUSH_ARRAY(arena, type, num)
    - Pushes `num` `type` structs onto `arena`
- MGA_PUSH_ZERO_ARRAY(arena, type, num)
    - Pushes `num` `type` structs onto `arena` and zeros the memory

### TODO
- Documentation
- Testing
- Article about implementation
