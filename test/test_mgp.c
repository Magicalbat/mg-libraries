#include <stdio.h>

#define MGP_STATIC
#define MG_PROFILE_IMPL
#include "../mg_profile.h"

int main(void) {
    mgp_init();

    uint64_t t0 = mgp_gettime(MGP_NANO_SEC);
    Sleep(2000);
    uint64_t t1 = mgp_gettime(MGP_NANO_SEC);

    printf("%llu\n", t1 - t0);

    return 0;
}
