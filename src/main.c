#include <stdio.h>
extern void cdynsplit_update();

#include "sub_host.dyn.gen.h"

int global_frame = 0;

int potentially_global () { return 111; }

int main (int argc, char** argv) {
    while (1) {
        global_frame += 1;
        printf(">>> Frame %i begin\n", global_frame);
        cdynsplit_update();
        sub_host_update();
        printf(">>> Frame %i end\n", global_frame);
    }

    return 0;
}
