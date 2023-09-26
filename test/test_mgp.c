
/*
Win32 Compile:
clang test/test_mgp.c test/mgp_impl.c -lgdi32 -luser32 -lopengl32 -o bin/test_mgp.exe

Linux Compile:
clang test/test_mgp.c test/mgp_impl.c -lX11 -lGL -lm -lGLX -o bin/test_mgp 
*/

#include <stdio.h>
#include <stdlib.h>

#include "../mg_plot.h"

int main(void) {
    mgp_init();
    
    mgp_enable_legend(MGP_ALIGN_LEFT);

    mgp_set_title(MGP_STR8("Plot"));
    mgp_set_win_size(400, 300);

    #define NUM 16
    float xs[NUM];
    float ys[NUM];

    for (int i = 0; i < NUM; i++) {
        xs[i] = i;
        ys[i] = (float)i + (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f);
    }

    mgp_lines_ex(NUM, xs, ys, 3.0f, MGP_LINE_SOLID, (mgp_vec4f){ 0 }, NULL, MGP_STR8("Line 1"));

    for (int i = 0; i < NUM; i++) {
        ys[i] = (float)i * 1.1f + (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f);
    }

    mgp_points_ex(NUM, xs, ys, 3.0f, (mgp_vec4f){ 0 }, NULL, MGP_STR8("Points"));

    mgp_plot_show();

    return 0;
}
