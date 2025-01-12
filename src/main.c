void upd_dyn();


#include "sub_host.dyn.gen.h"
#include "systems/kek.dyn.gen.h"

int main (int argc, char** argv) {
    while (1) {
        upd_dyn();
        sub_host_update();
    }

    return 0;
}
