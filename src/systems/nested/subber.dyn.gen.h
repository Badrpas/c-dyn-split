/**
 * File is generated
 **/

#ifndef SYSTEMS_NESTED_SUBBER_DYN_C
#define SYSTEMS_NESTED_SUBBER_DYN_C



#ifndef _DYN_SPLIT_BUILD

void subber_impl ();


#else

extern int printf (const char *__restrict __format, ...);

void reg_dyn (char* mpath, char* symbol, void** target);

static void (* __ptr_subber_impl) () = 0;
void subber_impl () {
  static int inited = 0; 
  if (!inited) {
    inited = 1; 
printf("[PROXY.init] initing subber_impl\n");
    reg_dyn("/home/grug/projects/anotherreloadc/out/systems/nested/subber.dyn.so", "subber_impl", (void*)&__ptr_subber_impl);
  }
  if (__ptr_subber_impl) {
    return __ptr_subber_impl();
  }
}


#endif

#endif
