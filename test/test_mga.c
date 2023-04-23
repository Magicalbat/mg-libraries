#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MGA_STATIC
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

void test_error_callback(mga_error err) { 
    printf("MGA Error %u: %s\n", err.code, err.msg);
}
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
    TEST_ASSERT(num_ptr != NULL, "Num ptr");
    *num_ptr = 42;

    int* num_arr = (int*)mga_push(arena, sizeof(int) * 64);
    TEST_ASSERT(num_arr != NULL, "num_arr");
    for (int i = 0; i < 64; i++) {
        num_arr[i] = 123;
    }

    int* zero_arr = (int*)mga_push_zero(arena, sizeof(int) * 256);
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT(zero_arr[i] == 0, "push zero");
    }

    float* test_float = MGA_PUSH_STRUCT(arena, float);
    TEST_ASSERT(test_float != NULL, "PUSH_STRUCT");
    *test_float = 3.14159f;
    
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

    char* large_alloc = (char*)mga_push(arena, MGA_KiB(512));
    TEST_ASSERT(large_alloc != NULL, "large alloc");

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

bool test_pop(void) {
    char* data = (char*)mga_push(arena, 1024);
    (void)data;
    mga_u64 start_pos = mga_get_pos(arena);

    mga_pop(arena, 1024);

    mga_u64 end_pos = mga_get_pos(arena);

    TEST_ASSERT(start_pos - end_pos == 1024, "pop");

    mga_reset(arena);
#ifdef MGA_MIN_POS
    TEST_ASSERT(mga_get_pos(arena) == MGA_MIN_POS, "reset");
#else
    TEST_ASSERT(mga_get_pos(arena) == 0, "reset");
#endif

    return true;
}

bool test_temp(void) {
    mga_u64 start_pos = arena->_pos;
    mga_temp temp = mga_temp_begin(arena);

    TEST_ASSERT(temp._pos == arena->_pos, "temp begin");

    mga_push(arena, arena->_block_size + 8);
    mga_push(arena, arena->_block_size + 8);

    mga_temp_end(temp);

    TEST_ASSERT(start_pos == arena->_pos, "temp end");

    return true;
}

bool test_destroy(void) {
    // I guess this only fails if there is a seg fault
    mga_destroy(arena);

    return true;
}

bool test_scratch(void) {
    mga_desc desc = {
        .desired_max_size = MGA_MiB(1),
        .error_callback = test_error_callback,
    };
    mga_scratch_set_desc(&desc);

    mga_temp scratch0 = mga_scratch_get(NULL, 0);

    TEST_ASSERT (
        scratch0.arena->_size >= desc.desired_max_size && 
        scratch0.arena->error_callback == test_error_callback,
        "scratch set desc"
    );
    
    mga_u64 spos0 = mga_get_pos(scratch0.arena);

    char* data0 = mga_push(scratch0.arena, MGA_KiB(512));
    TEST_ASSERT(data0 != NULL, "scratch push");

    mga_temp scratch1 = mga_scratch_get(&scratch0.arena, 1);
    TEST_ASSERT(scratch0.arena != scratch1.arena, "scratch conflicts");

    mga_u64 spos1 = mga_get_pos(scratch1.arena);

    char* data1 = mga_push(scratch1.arena, MGA_KiB(512));
    TEST_ASSERT(data1 != NULL, "scratch push");

    mga_scratch_release(scratch0);
    mga_scratch_release(scratch1);

    TEST_ASSERT(mga_get_pos(scratch0.arena) == spos0, "scratch release");
    TEST_ASSERT(mga_get_pos(scratch1.arena) == spos1, "scratch release");

    return true;
}

#define TEST_XLIST \
    X(MISC, misc) \
    X(CREATE, create) \
    X(PUSH, push) \
    X(GETTERS, getters) \
    X(POP, pop) \
    X(TEMP, temp) \
    X(DESTROY, destroy) \
    X(SCRATCH, scratch)

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
    bool quiet = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0)
            quiet = true;
    }

    uint32_t num_passed = 0;
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
    printf("Test Results: " GRN_BG("%d/%d passed") ", " RED_BG("%d/%d failed") ".\n",
        num_passed, TEST_COUNT, TEST_COUNT - num_passed, TEST_COUNT);
    
    return 0;
}
