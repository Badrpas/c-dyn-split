#include "stdio.h"

#include "dummy.h"
#include <inttypes.h> // pub
#include <dlfcn.h> // pub

extern void hosted();
void hosted3();

int kookoo[2];

void do_kek (int a, int BBB) {
    /* hosted(); */
    /* hosted2(); */
    /* hosted3(); */
    void* handle = dlopen("systems/top.dyn.so", RTLD_NOW);
    printf(LUL " + do_kek(%i, %i); top 7 handle = %p\n", a, BBB, handle);
    if (handle) {
        void (*top)() = dlsym(handle, "top");
        printf(" top= %p\n", top);
        if (top) {
            top();
        }
        dlclose(handle);
    }
}


typedef struct {
    int foo;
    int bar;
} RET;

