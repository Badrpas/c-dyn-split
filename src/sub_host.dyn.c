#include <stdio.h>
#include <unistd.h>

#include "systems/kek.dyn.gen.h"


int subhost_ret123 () {
    return 123;
}

int subhost_ret777 () {
    return 777;
}

int subhost_kokoko () {
    return 33;
}

int subhost_azaza() { return 12; }

int subhost_koka() {return 888888888;}

static int counter = 0;

void sub_host_update () {
    printf("> sub_host_update %p\n", &counter);
    do_kek(counter, counter + 1);
    counter += 2;

    sleep(5);
}


