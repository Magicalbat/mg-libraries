#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MG_ARENA_IMPL
#include "../mg_arena.h"

#define TEST_ASSERT(b, m) \
    if (!(b)) { printf("\x1b[35mAssert Failed: " m "\x1b[0m\n"); return false; }

static mg_arena* arena;

#define IS_POW2(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

bool test_misc(void) {
    TEST_ASSERT(MGA_KiB(1) == 1024, "KiB");
    TEST_ASSERT(MGA_KiB(2) == 2048, "KiB");
    TEST_ASSERT(MGA_MiB(1) == 1048576, "MiB");
    TEST_ASSERT(MGA_MiB(2) == 2097152, "MiB");
    TEST_ASSERT(MGA_GiB(1) == 1073741824, "GiB");
    TEST_ASSERT(MGA_GiB(2) == 2147483648, "GiB");

    TEST_ASSERT(sizeof(mga_i8 ) == 1, "mga_i8  size");
    TEST_ASSERT(sizeof(mga_i16) == 2, "mga_i16 size");
    TEST_ASSERT(sizeof(mga_i32) == 4, "mga_i32 size");
    TEST_ASSERT(sizeof(mga_i64) == 8, "mga_i64 size");
    TEST_ASSERT(sizeof(mga_u8 ) == 1, "mga_u8  size");
    TEST_ASSERT(sizeof(mga_u16) == 2, "mga_u16 size");
    TEST_ASSERT(sizeof(mga_u32) == 4, "mga_u32 size");
    TEST_ASSERT(sizeof(mga_u64) == 8, "mga_u64 size");
    TEST_ASSERT(sizeof(mga_b32) == 4, "mga_b32 size");

    return true;
}

void test_error_callback(mga_error err) { (void)err; }
bool test_create(void) {
    arena = mga_create(&(mga_desc){
        .desired_max_size = MGA_MiB(4),
        .desired_block_size = MGA_KiB(128),
        .align = sizeof(void*),
        .error_callback = test_error_callback
    });

    TEST_ASSERT(arena != NULL, "Arena create");
    
    mga_u64 size = mga_get_size(arena);
    TEST_ASSERT(IS_POW2(size) && size >= MGA_MiB(4), "Max size");
    
    mga_u32 block_size = mga_get_block_size(arena);
    TEST_ASSERT(IS_POW2(block_size) && block_size >= MGA_KiB(128), "Block size");
    
    mga_u32 align = mga_get_align(arena);
    TEST_ASSERT(IS_POW2(align) && align == sizeof(void*), "Align");

    TEST_ASSERT(arena->error_callback == test_error_callback, "Error callback");
    
    return true;
}

bool test_push(void) {
    int* num_ptr = (int*)mga_push(arena, sizeof(int));
    *num_ptr = 42;
    TEST_ASSERT(num_ptr != NULL, "Num ptr");

    int* num_arr = (int*)mga_push(arena, sizeof(int) * 64);
    for (int i = 0; i < 64; i++) {
        num_arr[i] = 123;
    }
    TEST_ASSERT(num_arr != NULL, "num_arr");

    int* zero_arr = (int*)mga_push_zero(arena, sizeof(int) * 256);
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT(zero_arr[i] == 0, "push zero");
    }

    float* test_float = MGA_PUSH_STRUCT(arena, float);
    *test_float = 3.14159f;
    TEST_ASSERT(test_float != NULL, "PUSH_STRUCT");
    
    float* zero_float = MGA_PUSH_ZERO_STRUCT(arena, float);
    TEST_ASSERT(test_float != NULL && *zero_float == 0.0f, "PUSH_STRUCT_ZERO");

    char* char_arr = MGA_PUSH_ARRAY(arena, char, 6);
    char_arr[0] = 'H';
    char_arr[1] = 'e';
    char_arr[2] = 'l';
    char_arr[3] = 'l';
    char_arr[4] = 'o';
    char_arr[5] = '\0';
    TEST_ASSERT(char_arr != NULL && strcmp(char_arr, "Hello") == 0, "PUSH_ARRAY");

    float* float_zeros = MGA_PUSH_ZERO_ARRAY(arena, float, 10);
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT(float_zeros[i] == 0, "PUSH_ZERO_ARRAY");
    }

    mga_error err = mga_get_error(arena);
    TEST_ASSERT(err.code == MGA_ERR_NONE, "got mga error");

    return true;
}

bool test_getters(void) {
    TEST_ASSERT(mga_get_pos(arena) == arena->_pos, "get pos");
    TEST_ASSERT(mga_get_size(arena) == arena->_size, "get size");
    TEST_ASSERT(mga_get_block_size(arena) == arena->_block_size, "get block_size");
    TEST_ASSERT(mga_get_align(arena) == arena->_align, "get align");

    return true;
}

bool test_destroy(void) {
    // I guess this only fails if there is a seg fault
    mga_destroy(arena);

    return true;
}

#define TEST_XLIST \
    X(MISC, misc) \
    X(CREATE, create) \
    X(PUSH, push) \
    X(GETTERS, getters) \
    X(DESTROY, destroy)

enum {
#define X(name, func_name) TEST_##name,
    TEST_XLIST
#undef X
    TEST_COUNT
};

static const char* test_names[TEST_COUNT] = {
#define X(name, func_name) #name,
    TEST_XLIST
#undef X
};

typedef bool (test_func)(void);
static test_func* test_funcs[TEST_COUNT] = {
#define X(name, func_name) test_##func_name,
    TEST_XLIST
#undef X
};

#define RED_BG(s) "\x1b[41m" s "\x1b[0m"
#define GRN_BG(s) "\x1b[42m" s "\x1b[0m"

int main(int argc, char** argv) {
    uint32_t num_passed = 0;
    bool quiet = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0)
            quiet = true;
    }

    for (int i = 0; i < TEST_COUNT; i++) {
        if (test_funcs[i]()) {
            if (!quiet)
                printf(GRN_BG("Test passed:") " %s\n", test_names[i]);
            
            num_passed++;
        } else {
            if (!quiet)
                printf(RED_BG("Test failed:") " %s\n", test_names[i]);
        }
    }

    if (!quiet) { puts(""); }
    printf("Test Results: " GRN_BG("%d/%d passed") ", " RED_BG("%d/%d failed") "\n",
        num_passed, TEST_COUNT, TEST_COUNT - num_passed, TEST_COUNT);
    
    return 0;
}
