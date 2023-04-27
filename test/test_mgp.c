#include <stdio.h>
#include <stdlib.h>

#define MGP_STATIC
#define MG_PROFILE_IMPL
#include "../mg_profile.h"

void test_func(mgp_u64 n, void* data){
    (void)data;

    Sleep(n);
}

int main(void) {
    mgp_init();
    
    mgp_input_size_desc profile_desc = {
        .func = test_func,
        .iters = 64,
        .input_start = 0,
        .time_unit = MGP_MILLI_SEC,
    };

    mgp_info info = mgp_profile_input_size(&profile_desc);

    printf("%f %f\n", info.average_time, info.total_time);

    //FILE* f = fopen("out.csv", "w");
    //mgp_multi_output_file(f, MGP_MFILE_CSV, &minfo);
    //fclose(f);
    
    return 0;
}
