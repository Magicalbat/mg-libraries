# MG Libraries

## STB-Style single header libraries

| Library | Docs | Description |
| ------- | ---- | ----------- |
| [mg_arena.h](mg_arena.h) | [MG Arena](docs/mg_arena.md) | Arena Memory Managment |
| [mg_plot.h](mg_plot.h) | [MG Plot](docs/mg_plot.md) | Plotting library |

## General Installation
Generally, to use one of these libraries, you should make a separate file (something like `mg_impl.c`), and put the following in the file:
```c
#define MG_*_IMPL
#include "mg_*.h"
```
Some libraries may require more steps and/or custom compile instructions.


## [MG Arena](mg_arena.h) ([Docs](docs/mg_arena.md))
A small library for memory arenas in C.

Example:
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


## [MG Plot](mg_plot.h) ([Docs](docs/mg_plot.md))
A library for creating plots. It is basically a C imitaion of [matplotlib](https://matplotlib.org/) (with way less features).

**NOTE: This library requires `mg_arena.h` to work properly. There is also an optional feature that requires `stb_image_write.h` to work properly.**

**Build Instructions**
- Create a source file for the implementation:
```c
#include "mg_arena.h"

// Optionally
#include "stb_image_write.h"

#define MG_PLOT_IMPL
#include "mg_plot.h"
```
- Compile
    - Windows: Link with gdi32.lib, user32.lib, and opengl32.lib
        - `clang main.c mg_impl.c -lgdi32 -luser32 -lopengl32 -o main.exe`
    - Linux: Link with m, X11, GL, and GLX
        - `clang main.c mg_impl.c -lm -lX11 -lGL -lGLX -o main`

Example:

![MG Plot Example Image](/docs/res/mgp_example.png)

mg_impl.c:
```c
// Required by mg_plot.h
#include "mg_arena.h"

// Optionally
#include "stb_image_write.h"

#define MG_ARENA_IMPL
#include "mg_arena.h"

#define MG_PLOT_IMPL
#include "mg_plot.h"
```

main.c:
```c
#include <stdlib.h>

#include "mg_plot.h"

int main(void) {
    mgp_init();
    
    mgp_enable_legend(MGP_ALIGN_LEFT);

    mgp_set_title(MGP_STR8("Plot"));

    #define NUM_POINTS 16
    float xs[NUM_POINTS];
    float ys[NUM_POINTS];

    for (int i = 0; i < NUM_POINTS; i++) {
        xs[i] = i;
        ys[i] = (float)i + (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f);
    }

    mgp_lines_ex(NUM_POINTS, xs, ys, 3.0f, MGP_LINE_SOLID, (mgp_vec4f){ 0 }, NULL, MGP_STR8("Line 1"));

    for (int i = 0; i < NUM_POINTS; i++) {
        ys[i] = (float)i * 1.1f + (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f);
    }

    mgp_lines_ex(NUM_POINTS, xs, ys, 3.0f, MGP_LINE_SOLID, (mgp_vec4f){ 0 }, NULL, MGP_STR8("Line 2"));
    
    for (int i = 0; i < NUM_POINTS; i++) {
        ys[i] = (float)i + (((float)rand() / (float)RAND_MAX) * 4.0f - 2.0f);
    }

    mgp_points_ex(NUM_POINTS, xs, ys, 2.0f, (mgp_vec4f){ 0 }, NULL, MGP_STR8("Points"));

    mgp_plot_show();

    return 0;
}
```
