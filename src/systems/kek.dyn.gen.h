/**
 * File is generated
 **/
#include <dlfcn.h>
#include <inttypes.h>

#ifndef _DYN_SPLIT_BUILD

void do_kek (int a, int BBB);


#else


void reg_dyn (char* mpath, char* symbol, void** target);

void (* __ptr_do_kek) (int a, int BBB) = 0;
void do_kek (int a, int BBB) {
  static int inited = 0; 
  if (!inited) {
    inited = 1; 
    reg_dyn("/home/grug/projects/anotherreloadc/out/systems/kek.dyn.so", "do_kek", (void*)&__ptr_do_kek);
  }
  if (__ptr_do_kek) {
    return __ptr_do_kek(a, BBB);
  }
}



#endif
