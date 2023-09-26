# MG Plot

## NOTE: This should be considered a pre-alpha or alpha librry. I have not extensively tested this library and there are some significant naming issues within `mg_plot.h`.
(That is also why there is no static option).


A library for creating plots. It is basically a C imitaion of [matplotlib](https://matplotlib.org/) (with way less features).

MG Plot requires `mg_arena.h` for memory managment, OpenGL for rendering, and, optionally, `stb_image_write.h` for writing images.`

## Documentation

- [Building](#building)
- [Example](#example)
- [Introduction](#introduction)
- [Plot Controls](#plot-controls)
- [Typedefs](#typedefs)
- [Enums](#enums)
- [Structs](#structs)
- [Functions](#functions)
- [Macros](#macros)

Building
--------

First, create a c file to hold the implementation. This example also implements `mg_arena.h` in the same file. 

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

**IMPORTANT: Because `mg_plot.h` required OpenGL for rendering, you must link to to necessary libraries for each platform.**

### Windows
Link with gdi32.lib, user32.lib, and opengl32.lib

`clang main.c mg_impl.c -lgdi32 -luser32 -lopengl32 -o main.exe`

### Linux
Link with m, X11, GL, and GLX

`clang main.c mg_impl.c -lm -lX11 -lGL -lGLX -o main`

Example
-------

![MG Plot Example Image](/docs/res/mgp_example.png)

(mg_impl file not shown)
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


Introduction
------------

To get started with `mg_plot.h`, first create the implementation file as shown above. Make sure that everything is linking correctly. 

Using `mg_plot.h` is broken up into the following steps:

1) Call `mgp_init()`
2) Change any settings that you might want to. This includes the plot title, window size, color scheme, and whether or not to show a legend.
3) Add as many draw commands as you want. This can be done through the helper functions `plot_points_ex`, `plot_lines_ex`, `plot_rects_ex`, and `plot_quads_ex`. You can also call the simpler versions without the "_ex". For the most control, you can make a `mgp_draw_cmd` struct and call `mgp_cmd_push`.
4) Call `mgp_plot_show()` to display the window. 

Plot Controls
-------------

- House: The first button on the left resets the view to the initial view.
- Crosshair: The second button enabled panning mode. This allows you to drag you mouse on the graph to move the view.
- Magnifying Glass: The third button enabled zooming mode. Left click and drag to zoom in, right click to zoom out. 
- Save: The final button will save the plot to a PNG image. This button will only work if you include `stb_image_write.h` before the implementation.

Typedefs
--------

- `mgp_i8`
    - 8 bit signed integer
- `mgp_i32`
    - 32 bit signed integer
- `mgp_i64`
    - 64 bit signed integer
- `mgp_u8`
    - 8 bit unsigned integer
- `mgp_u16`
    - 16 bit unsigned integer
- `mgp_u32`
    - 32 bit unsigned integer
- `mgp_u64`
    - 64 bit unsigned integer
- `mgp_b8`
    - 8 bit boolean
- `mgp_b32`
    - 32 bit boolean
- `mgp_f32`
    - 32 bit floating point
- `mgp_f64`
    - 64 bit floating point

Enums
-----
- `MGP_LINE_SOLID` and `MGP_LINE_DASHED`
    - Draw type of line command
- `MGP_DRAW_POINTS`, `MGP_DRAW_LINES`, `MGP_DRAW_RECTS`, and `MGP_DRAW_QUADS`
    - Type of draw command
- `mgp_legend_align`: `MGP_ALIGN_RIGHT` and `MGP_ALIGN_LEFT`
    - Alignment of legend

Structs
-------
- `mgp_string8`
    - 8 bit, length based string, see `MGP_STR8` macro
    - `mgp_u64` *size*
        - This does not include any null terminator 
    - `mgp_u8*` *str*
        - This does not need to have a null terminator

<br>

- `mgp_vec2f`
    - `mgp_f32` *x*, *y*
- `mgp_vec3f`
    - `mgp_f32` *x*, *y*, z*
- `mgp_vec4f`
    - `mgp_f32` *x*, *y*, *z*, *w*

<br>
    
- `mgp_recti`
    - `mgp_i32` *x*, *y*, *w*, *h*
- `mgp_rectf`
    - `mgp_f32` *x*, *y*, *w*, *h*
- `mgp_quadf` (union)
    - `mgp_vec2f` *p[4]*;
    - struct
        - `mgp_vec2f` *p0*
        - `mgp_vec2f` *p1*
        - `mgp_vec2f` *p2*
        - `mgp_vec2f` *p3*

<br>

- `mgp_points_cmd`
    - `mgp_f32` *size*
        - size of points
    - `mgp_vec2f*` *data*
- `mgp_lines_cmd`
    - `enum` *type*
        - `MGP_LINE_SOLID`
        - `MGP_LINE_DASHED`
    - `mgp_f32` *width*
        - width of line
    - `mgp_vec2f*` *data*
- `mgp_rects_cmd`
    - `mgp_rectf*` *data*
- `mgp_quads_cmd`
    - `mgp_quadf*` *data*
- `mgp_draw_cmd`
    - `mgp_vec4f` *color*
        - If *color* is all zero, the next color will be chosen based on *default_draw* from the color scheme
    - `mgp_vec4f*` *colors*
        - Colors can be NULL
        - If not, it has to be of size `size`, and it will be used before *color*
    - `mgp_string8` *label*
        - Option label
        - It will only be shown if the legend in enabled
        - Use `MGP_STR8` to make `mgp_string8` from string literal
    - `enum`*type*
        - `MGP_DRAW_POINTS`
        - `MGP_DRAW_LINES`
        - `MGP_DRAW_RECTS`
        - `MGP_DRAW_QUADS`
    - anonymous union
        - `mgp_points_cmd` *points*
        - `mgp_lines_cmd` *lines*
        - `mgp_rects_cmd` *rects*
        - `mgp_quads_cmd` *quads*

<br>

- `mgp_color_scheme`
    - `mgp_vec4f` *background*
        - Background of main area above buttons
    - `mgp_vec4f` *graph_background*
        - Background of graph area
    - `mgp_vec4f` *graph_outline*
        - 1 pixel outline of graph area
    - `mgp_vec4f` *axes_text*
        - Title and axes text
    - `mgp_vec4f` *legend_background*
        - Background of legend box
    
    - `mgp_vec4f` *legend_outline*
        - Outline of legend box
    - `mgp_vec4f` *legend_text*
        - Color of legend text

    - `mgp_vec4f` *bottom_strip*
        - Background color of bottom strip (where buttons are)
    - `mgp_vec4f` *button_foreground*
        - Color of button icons
    - `mgp_vec4f` *button_background*
        - Background color of button icons
    - `mgp_vec4f` *button_hovered*
        - Background color of hovered button
    - `mgp_vec4f` *button_toggled*
        - Background color of toggled button

    - `mgp_vec4f` *zoom_rect*
        - Color of zoom rect

    - `mgp_vec4f` *default_draw[8]*
        - Default colors for draw commands

- `mgp_view`
    - `mgp_f32` left 
    - `mgp_f32` right 
    - `mgp_f32` top 
    - `mgp_f32` bottom 

Functions
---------

- `void mgp_init()`
    - You must call this before any other function
    - Initializes the plotting system
- `void mgp_cmd_push(const mgp_draw_cmd* cmd)`
    - Add a draw command to the plot
    - See the struct `mgp_draw_cmd` for more detail
- `void mgp_plot_show()`
    - Opens the plot window

<br>

All of these below should be called before any draw commands

- `void mgp_enable_legend(mgp_legend_align align)`
    - Enables the legend on the plot and sets the align
- `void mgp_set_color_scheme(const mgp_color_scheme* colors)`
    - Updates the color scheme
    - Any colors set to `(mgp_vec4f){ 0 }` will take their default value
- `void mgp_set_view(mgp_view new_view)`
    - Sets the initial view of the plot
    - See struct `mgp_view`
    - This will also be the view that the home button uses
- `void mgp_set_win_size(mgp_u32 width, mgp_u32 height)`
    - Sets the size of the window
- `void mgp_set_title(mgp_string8 title)`
    - This will be the title on the window, plot, and any saved images
    - See `MGP_STR8` for creating `mgp_string8`

<br>

These functions construct a `mgp_draw_cmd` frmo the arguments and call `mgp_cmd_push`

- `void mgp_points_ex(mgp_u32 num_points, mgp_f32* xs, mgp_f32* ys, mgp_f32 size, mgp_vec4f color, mgp_vec4f* colors, mgp_string8 label)`
    - `num_points`: the size of `xs`, and `ys`
    - `xs`: list of x values
    - `ys`: list of y values
    - `size`: size of points
    - `color`: color of points
        - If `(mgp_vec4f){ 0 }`, a default color will be used
    - `colors`: each color of the points
        - Can be `NULL`
        - Will be used before `color`
    - `label`: display name of points on legend
        - If size is zero, it will not be displayed in the legend
        - If the legend is not enabled, it will not be displayed
- `void mgp_lines_ex(mgp_u32 num_points, mgp_f32* xs, mgp_f32* ys, mgp_f32 width, mgp_u32 type, mgp_vec4f color, mgp_vec4f* colors, mgp_string8 label)`
    - `width`: width of line in pixels
    - The rest is the same as `mgp_points_ex`
- `void mgp_rects_ex(mgp_u32 num_rects, mgp_rectf* rects, mgp_vec4f color, mgp_vec4f* colors, mgp_string8 label)`
    - `num_rects`: number of rects in `rects`
    - `rects`: list of rects
    - `color`, `colors`, and `label` is the same as `mgp_points_ex`
- `void mgp_quads_ex(mgp_u32 num_quads, mgp_quadf* quads, mgp_vec4f color, mgp_vec4f* colors, mgp_string8 label)`
    - `num_quads`: number of quads in `quads`
    - `quads`: list of quads
    - `color`, `colors`, and `label` is the same as `mgp_points_ex`

Macros
------
- `MGP_STR8(s)`
    - Creates an `mgp_string8` from a string literal `s`
- `mgp_points(num_points, xs, ys)`
    - Calls mgp_points_ex with reasonable defaults
    - `mgp_u32` num_points
    - `mgp_f32*` xs
    - `mgp_f32*` ys
- `mgp_lines(num_points, xs, ys)`
    - Calls mgp_lines_ex with reasonable defaults
    - `mgp_u32` num_points
    - `mgp_f32*` xs
    - `mgp_f32*` ys
- `mgp_rects(num_rects, rects)`
    - Calls mgp_rects_ex with reasonable defaults
    - `mgp_u32` num_rects
    - `mgp_rectf*` rects
- `mgp_quads(num_quads, quads)`
    - Calls mgp_quads_ex with reasonable defaults
    - `mgp_u32` num_quads
    - `mgp_quadf*` quads
    
