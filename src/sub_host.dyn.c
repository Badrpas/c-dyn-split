#include <stdint.h> // pub
#include <stdio.h>
#include <unistd.h>

#include "systems/kek.dyn.gen.h"


int subhost_ret123 () {
    return 123;
}

static int counter = 0;

void sub_host_update () {
    printf("> sub_host_update %p\n", &counter);
    do_kek(counter, counter + 1);
    counter += 2;

    sleep(5);
}


