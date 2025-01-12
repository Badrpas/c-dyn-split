#define _DYN_SPLIT_BUILD

#ifdef _DYN_SPLIT_BUILD

#include "stdio.h"
#include <string.h>
#include <time.h>
#include <assert.h>
#include <dlfcn.h>
#include <linux/limits.h>

typedef struct {
    struct {
        char* module_path;
        char* symbol;
        void** bind_target;
    } entries[100];
    struct {
        char* module_path;
        void* handle;
        time_t loaded_ts;
        time_t created_at;
    } modules[100];
} DYN_HANDLER;

DYN_HANDLER dyn_handler = {0};

struct {
    const char *dli_fname;  /* Pathname of shared object that
                               contains address */
    void       *dli_fbase;  /* Address at which shared object
                               is loaded */
    const char *dli_sname;  /* Name of nearest symbol with address
                               lower than addr */
    void       *dli_saddr;  /* Exact address of symbol named
                               in dli_sname */
} __dl_info = {0};

static char* err = 0;
#define CHECK_ERR(stage) \
do { err = dlerror(); if (err) printf("[DL_ERROR] while [" #stage "]: %s\n", err); } while(0)

void upd_dyn () {
    time_t now = time(0);
    for (int module_idx = 0; module_idx < 100; module_idx++) {
        typeof(dyn_handler.modules[module_idx])* m = &dyn_handler.modules[module_idx];
        if (!m->module_path) {
            continue;
        }
        double diff_seconds = difftime(now, m->loaded_ts);
        if (diff_seconds < 10) {
            continue;
        }
        printf("[UPD] Updating %.2f %s\n", diff_seconds, m->module_path);

        m->loaded_ts = now;
        void* prev_handle = m->handle;
        if (m->handle) {
            printf("[UPD] closing %p\n", m->handle);
            dlclose(m->handle);
            CHECK_ERR(closing);
        }

        m->handle = dlopen(m->module_path, RTLD_LAZY);
        CHECK_ERR(opening);
        if (prev_handle != m->handle) {
            printf("[UPD] handle changed %p -> %p\n", prev_handle, m->handle);
        } else {
            printf("[UPD] handle unchanged\n");
        }

        for (int entry_idx = 0; entry_idx < 100; entry_idx++) {
            typeof(dyn_handler.entries[entry_idx])* entry = &dyn_handler.entries[entry_idx];
            if (!entry->module_path)
                continue;
            if (!strcmp(m->module_path, entry->module_path)) {
                assert(entry->bind_target);
                void* target = dlsym(m->handle, entry->symbol);
                CHECK_ERR(symbol_resolve);
                if (*entry->bind_target == target) {
                    printf("[UPD] Unchanged symbol [%i] %s: %p\n", entry_idx, entry->symbol, *entry->bind_target);
                } else {
                    printf("[UPD] Updating symbol [%i] %s: %p -> %p\n", entry_idx, entry->symbol, *entry->bind_target, target);
                }
                *(entry->bind_target) = target;
            }
        }
    }
}

void reg_dyn (char* mpath, char* symbol, void** target) {
    printf("[DYN.REG] Registering %s : %s -> %p\n", mpath, symbol, target);
    for (int i = 0; i < 100; i++) {
        typeof(dyn_handler.entries[i])* entry = &dyn_handler.entries[i];
        if (entry->module_path) {
            if (!strcmp(entry->module_path, mpath)) {
                if (!strcmp(entry->symbol, symbol)) {
                    if (entry->bind_target == target) {
                        printf("[DYN.REG] %p already registered\n", target);
                        return;
                    }
                }
            }
            continue;
        }
        entry->module_path = mpath;
        entry->symbol = symbol;
        entry->bind_target = target;
        printf("[DYN.REG] Added entry at %i\n", i);
        for (int j = 0; j < 100; j++) {
            if (dyn_handler.modules[j].module_path) {
                if (!strcmp(dyn_handler.modules[j].module_path, mpath)) {
                    printf("[DYN.REG] found the module; returning\n");
                    return;
                }
                continue;
            }
            dyn_handler.modules[j].module_path = mpath;
            dyn_handler.modules[j].handle = 0;
            dyn_handler.modules[j].loaded_ts = 0;
            printf("[DYN.REG] Added module at %i\n", j);
            upd_dyn();
            break;
        }
        return;
    }
    // no free estate
    assert(0);
}

#else
void upd_dyn () {}
void reg_dyn (char* mpath, char* symbol, void** target) {}
#endif
