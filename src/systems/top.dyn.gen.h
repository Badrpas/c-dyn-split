/**
 * File is generated
 **/

#ifndef SYSTEMS_TOP_DYN_C
#define SYSTEMS_TOP_DYN_C



#ifndef _DYN_SPLIT_BUILD

void top();


#else

extern int printf (const char *__restrict __format, ...);

void reg_dyn (char* mpath, char* symbol, void** target);

static void (* __ptr_top)() = 0;
void top() {
  static int inited = 0; 
  if (!inited) {
    inited = 1; 
printf("[PROXY.init] initing top\n");
    reg_dyn("/home/grug/projects/anotherreloadc/out/systems/top.dyn.so", "top", (void*)&__ptr_top);
  }
  if (__ptr_top) {
    return __ptr_top();
  }
}


#endif

#endif
