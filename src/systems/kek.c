
#include "stdio.h"

#include "dummy.h"
#include <dlfcn.h>

extern void hosted();
void hosted3();

void do_kek (int a, int b) {
    /* hosted(); */
    /* hosted2(); */
    /* hosted3(); */
    void* handle = dlopen("systems/top.so", RTLD_NOW);
    printf(LUL " + %i, %i %p\n", a, b, handle);
    if (handle) {
        void (*top)() = dlsym(handle, "top");
        printf(" top= %p\n", top);
        if (top) {
            top();
        }
        dlclose(handle);
    }
}
