#include <stdio.h>
void upd_dyn();


#include "sub_host.dyn.gen.h"

int main (int argc, char** argv) {
    int frame = 0;
    while (1) {
        frame += 1;
        printf(">>> Frame %i begin\n", frame);
        upd_dyn();
        sub_host_update();
        printf(">>> Frame %i end\n", frame);
    }

    return 0;
}
