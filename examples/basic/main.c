#include <stdio.h>
#include <unistd.h>

extern void cdynsplit_update();

#include "sub.dyn.gen.h"


int main (int argc, char** argv) {
    while (1) {
        cdynsplit_update();
        thisisdynamicfoo();
        printf("Hello from main.c loop\n");
        sleep(1);
    }

    return 0;
}

