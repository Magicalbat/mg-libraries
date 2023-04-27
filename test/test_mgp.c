#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#define MGP_STATIC
#define MG_PROFILE_IMPL
#include "../mg_profile.h"

void test_func(mgp_u64 n, void* data){
    (void)data;

    usleep(n * 10);
}

int main(void) {
    mgp_init();

    mgp_multi_info minfo = { 0 };
    
    mgp_input_size_desc profile_desc = {
        .func = test_func,
        .iters = 64,
        .input_start = 0,
        .time_unit = MGP_MICRO_SEC,
        .per_iter = 64,
        .multi_out = &minfo,
    };

    mgp_profile_multi_input_size(&profile_desc);

    FILE* f = fopen("out.csv", "w");
    mgp_multi_output_file(f, MGP_MFILE_CSV, &minfo);
    fclose(f);
    
    return 0;
}
