#include <inttypes.h> // pub
#include <dlfcn.h> // pub
#include "stdio.h"
#include "dummy.h"

#include "../sub_host.dyn.gen.h"

void do_kek (int a, int BBB, int c) {
    printf(LUL " + do_kek(%i, %i); ehehe --- %i\n", a, BBB, subhost_ret777());
    /* void* handle = dlopen("systems/top.dyn.so", RTLD_NOW); */
    /* if (handle) { */
        /* void (*top)() = dlsym(handle, "top"); */
        /* printf(" top= %p\n", top); */
        /* if (top) { */
            /* top(); */
        /* } */
        /* dlclose(handle); */
    /* } */
}

