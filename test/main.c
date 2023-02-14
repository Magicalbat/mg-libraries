#include <stdio.h>

#define MG_ARENA_IMPL
#include "../mg_arena.h"

void err_callback(mga_error_code code, char* msg) {
    fprintf(stderr, "MGA Error %d: %s\n", code, msg);
}

int main() {
    mg_arena* arena = mga_create(&(mga_desc){
        .desired_max_size = MGA_MiB(4),
        .desired_block_size = MGA_KiB(128),
        .error_callback = err_callback
    });

    int* nums = MGA_PUSH_ZERO_ARRAY(arena, int, 64);
    for (int i = 0; i < 64; i++) {
        nums[i] = i;
    }

    mga_reset(arena);

    mga_destroy(arena);

    return 0;
}
