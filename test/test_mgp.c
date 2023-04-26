#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#define MGP_STATIC
#define MG_PROFILE_IMPL
#include "../mg_profile.h"

void test_func(void* data){
    (void)data;

    uint32_t x = rand();
    x += 10;
}

int main(void) {
    mgp_init();

    mgp_multi_info multi_info = { 0 };

    //mgp_info pinfo = mgp_profile_basic(test_func, NULL, 1024, MGP_MICRO_SEC);
    mgp_profile_multi_basic(test_func, NULL, 128, 1024, MGP_MICRO_SEC, &multi_info);

    printf("{\n");
    for (uint32_t i = 0; i < multi_info.size; i++) {
        printf("\t %.0f, %f,\n", multi_info.total_times[i], multi_info.average_times[i]);
    }
    printf("}\n");

    return 0;
}
