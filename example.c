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