/**
 * File is generated
 **/

#ifndef SUB_HOST_DYN_C
#define SUB_HOST_DYN_C



#ifndef _DYN_SPLIT_BUILD

void sub_host_update ();
int subhost_ret123 ();
int subhost_ret777 ();


#else

extern int printf (const char *__restrict __format, ...);

void reg_dyn (char* mpath, char* symbol, void** target);

static void (* __ptr_sub_host_update) () = 0;
void sub_host_update () {
  static int inited = 0; 
  if (!inited) {
    inited = 1; 
printf("[PROXY.init] initing sub_host_update\n");
    reg_dyn("/home/grug/projects/anotherreloadc/out/sub_host.dyn.so", "sub_host_update", (void*)&__ptr_sub_host_update);
  }
  if (__ptr_sub_host_update) {
    return __ptr_sub_host_update();
  }
}

static int (* __ptr_subhost_ret123) () = 0;
int subhost_ret123 () {
  static int inited = 0; 
  if (!inited) {
    inited = 1; 
printf("[PROXY.init] initing subhost_ret123\n");
    reg_dyn("/home/grug/projects/anotherreloadc/out/sub_host.dyn.so", "subhost_ret123", (void*)&__ptr_subhost_ret123);
  }
  if (__ptr_subhost_ret123) {
    return __ptr_subhost_ret123();
  }
  int ___ret = {0};
  return ___ret;
}

static int (* __ptr_subhost_ret777) () = 0;
int subhost_ret777 () {
  static int inited = 0; 
  if (!inited) {
    inited = 1; 
printf("[PROXY.init] initing subhost_ret777\n");
    reg_dyn("/home/grug/projects/anotherreloadc/out/sub_host.dyn.so", "subhost_ret777", (void*)&__ptr_subhost_ret777);
  }
  if (__ptr_subhost_ret777) {
    return __ptr_subhost_ret777();
  }
  int ___ret = {0};
  return ___ret;
}


#endif

#endif
