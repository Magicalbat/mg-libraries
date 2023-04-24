#include <stdio.h>

#include <unistd.h>

#define MGP_STATIC
#define MG_PROFILE_IMPL
#include "../mg_profile.h"

int main(void) {
    mgp_init();

    mgp_time_unit unit = MGP_MICRO_SEC;
    uint64_t t0 = mgp_gettime(unit);
    usleep(2e6);
    uint64_t t1 = mgp_gettime(unit);

    printf("%lu\n", t1 - t0);

    return 0;
}
