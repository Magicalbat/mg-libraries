#include <stdio.h>

#define MG_ARENA_IMPL
#define MGA_FORCE_MALLOC
#include "../mg_arena.h"

// TODO: make actual tests

void err_callback(mga_error_code code, char* msg) {
    fprintf(stderr, "MGA Error %d: %s\n", code, msg);
}

int main() {
    printf("KB: %llu, MB: %llu, GB: %llu, KiB: %llu, MiB: %llu, GiB: %llu",
        MGA_KB(1), MGA_MB(1), MGA_GB(1),
        MGA_KiB(1), MGA_MiB(1), MGA_GiB(1)
    );
    
    mg_arena* arena = mga_create(&(mga_desc){
        .desired_max_size = MGA_MiB(4),
        .desired_block_size = MGA_KiB(128),
        .error_callback = err_callback
    });

    printf("%llu\n", mga_get_pos(arena));
    int* nums = MGA_PUSH_ZERO_ARRAY(arena, int, MGA_KiB(64));
    for (int i = 0; i < 64; i++) {
        nums[i] = i;
    }
    printf("%llu\n", mga_get_pos(arena));

    mga_reset(arena);
    
    printf("%llu\n", mga_get_pos(arena));
    nums = MGA_PUSH_ZERO_ARRAY(arena, int, 64);
    for (int i = 0; i < 64; i++) {
        nums[i] = i;
    }
    printf("%llu\n", mga_get_pos(arena));

    mga_destroy(arena);

    return 0;
}
