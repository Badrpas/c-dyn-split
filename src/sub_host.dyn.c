#include <stdio.h>
#include <unistd.h>

#include "systems/kek.dyn.gen.h"


int subhost_ret123 () {
    return 111;
}

int subhost_ret777 () {
    return 88;
}

static int counter = 0;

void sub_host_update () {
    printf("sub_host_update %p\n", &counter);
    do_kek(counter, counter + 1, 333);
    counter += 2;

    sleep(5);
}


