// Implementation can also go in a different source file
#define MG_ARENA_IMPL
#include "../mg_arena.h"

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
