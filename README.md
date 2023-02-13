# Magic Arena
## Project Status: Personal Education Project

An [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) library for creating [memory arenas](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).

### Inspired By
- [STB Libraries](https://github.com/nothings/stb)
- [Sokol Libraries](https://github.com/floooh/sokol)
- [Metadesk](https://github.com/Dion-Systems/metadesk)

### How to use
Download the file `mg_arena.h`. Create a source file for the implementation. Add the following:
```c
#define MG_ARENA_IMPL
#include "mg_arena.h"
```

### Sample

```c
// Implementation can also go in a different source file
#define MG_ARENA_IMPL
#include "mg_arena.h"

int main() {
    mg_arena* arena = mga_create(&(mga_desc){
        .max_size = MGA_MiB(4),
        .pages_per_block = 8,
    });

    int* nums = MGA_PUSH_ZERO_ARRAY(arena, int, 64);
    for (int i = 0; i < 64; i++) {
        nums[i] = i;
    }

    unsigned char* data = (unsigned char*)mga_push_zero(arena, sizeof(unsigned char) * 128);

    mga_destroy(arena);

    return 0;
}
```

### TODO
- Linked list arenas with malloc
- Documentation
- Testing
- Article about implementation
