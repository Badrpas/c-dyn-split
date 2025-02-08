#include <stdio.h>

extern void cdynsplit_update();

#include "sub.dyn.gen.h"


int main (int argc, char** argv) {
    while (1) {
        cdynsplit_update();
        thisisdynamicfoo();
        printf("Hello from main.c loop\n");
    }

    return 0;
}

