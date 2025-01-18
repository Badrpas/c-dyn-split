#include <unistd.h>
#define _DYN_SPLIT_BUILD
#ifdef _DYN_SPLIT_BUILD

#include "stdio.h"
#include <assert.h>
#define __USE_GNU
#include <dlfcn.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define UPDATE_INTERVAL_SECONDS 1.0

#define MAX_MODULES 1024
#define MAX_BINDINGS 1024
#define MAX_NAME_LEN 255

void* dlmopen(Lmid_t lmid, const char* filename, int flags);

struct {
    const char* mname; /* Pathname of shared object that
                          contains address */
    void* base;        /* Address at which shared object
                          is loaded */
    const char* sname; /* Name of nearest symbol with address
                          lower than addr */
    void* saddr;       /* Exact address of symbol named
                          in dli_sname */
} __dl_info = {0};
static typeof(__dl_info)* dl_info = &__dl_info;
/* int dladdr(void* addr, typeof(__dl_info)* info); */

typeof(dl_info) sinfo (void* addr) {
    if (dladdr(addr, (Dl_info*)dl_info)) {
        return dl_info;
    }
    return 0;
}

typedef struct {
    int module_idx;
    char symbol[MAX_NAME_LEN + 1];
    void** bind_target;
    void* registrar;
    int registrar_module_idx;
} Binding;
typedef struct {
    char module_path[MAX_NAME_LEN + 1];
    void* handle;
    time_t update_ts;
    time_t created_at;
} Module;

typedef struct {
    Binding binds[MAX_BINDINGS];
    Module modules[MAX_MODULES];
} DYN_HANDLER;

DYN_HANDLER Reg = {0};

static Module* find_module (const char* m_name) {
    for (int i = 1; i < MAX_MODULES; i++) {
        if ((m_name && !strcmp(Reg.modules[i].module_path, m_name)) || (!m_name && !Reg.modules[i].handle)) {
            return &Reg.modules[i];
        }
    }
    return 0;
}

static Binding* find_binding (char* m_name, char* e_symbol, void** target) {
    for (int i = 1; i < MAX_BINDINGS; i++) {
        Binding* b = &Reg.binds[i];
        if (e_symbol && !strcmp(b->symbol, e_symbol) && m_name && b->module_idx &&
            !strcmp(Reg.modules[b->module_idx].module_path, m_name) && target && target == b->bind_target) {
            return b;
        }
    }
    return 0;
}
static Binding* acquire_binding () {
    for (int i = 1; i < MAX_BINDINGS; i++) {
        Binding* b = &Reg.binds[i];
        if (!b->module_idx)
            return b;
    }
    assert(0);
    return 0;
}

static int calc_module_idx (Module* m) { return ((long long)(void*)m - (long long)(void*)&Reg.modules) / sizeof(*m); }
static int calc_binding_idx (Binding* b) { return ((long long)(void*)b - (long long)(void*)&Reg.binds) / sizeof(*b); }

static char* err = 0;
#define CHECK_ERR(stage)                                        \
    do {                                                        \
        err = dlerror();                                        \
        if (err)                                                \
            printf("[DL_ERROR] while [" #stage "]: %s\n", err); \
    } while (0)

static int update_needed = 0;

void cdynsplit_update () {
    time_t now = time(0);
    for (int module_idx = 1; module_idx < MAX_MODULES; module_idx++) {
        Module* m = &Reg.modules[module_idx];
        if (!m->module_path[0]) {
            continue;
        }
        double diff_seconds = difftime(now, m->update_ts);
        if (diff_seconds < UPDATE_INTERVAL_SECONDS) {
            continue;
        }
        m->update_ts = now;

        struct stat out = {0};
        if (stat(m->module_path, &out) != 0) {
            printf("[UPD.ERR] Couldn't stat %s\n", m->module_path);
            continue;
        }
        time_t so_created_at = out.st_ctim.tv_sec;

        if (m->created_at >= so_created_at) {
            continue;
        }
        double creation_diff = difftime(so_created_at, m->created_at);
        m->created_at = so_created_at;

        printf("[UPD] Updating %.2f %s\n", creation_diff, m->module_path);

        void* prev_handle = m->handle;
        if (m->handle) {
            int midx = calc_module_idx(m);
            if (midx) {
                // Try to unregister all symbols defined by the unloading module
                // This won't work all the time. It might be OK to keep stale references for time being...
                for (int i = 1; i < MAX_BINDINGS; i++) {
                    Binding* b = &Reg.binds[i];
                    if (b->registrar_module_idx && b->registrar_module_idx == midx) {
                        printf("[UPD.unload] Clearing binding idx=%i sym=%s m=%s registrar=%p\n", i, b->symbol,
                               m->module_path, b->registrar);
                        *b = (Binding){0};
                    }
                }
            } else {
                printf("[UPD.??] Couldn't find midx of m=%s\n", m->module_path);
            }
            dlclose(m->handle);
            CHECK_ERR(CLOSING);
        }

        m->handle = dlopen(m->module_path, RTLD_NOW);
        CHECK_ERR(OPENING);
        if (prev_handle != m->handle) {
            printf("[UPD] handle changed %p -> %p\n", prev_handle, m->handle);
        }
        update_needed = 1;
    }

    if (!update_needed) {
        printf("[UPD] Nothing changed\n");
        return;
    } else {
        /* printf("[UPD] Changes occured\n"); */
    };

    update_needed = 0;

    for (int i = 1; i < MAX_BINDINGS; i++) {
        Binding* b = &Reg.binds[i];
        if (!b->module_idx)
            continue;
        Module* m = &Reg.modules[b->module_idx];
        void* sym = dlsym(m->handle, b->symbol);
        CHECK_ERR(SYMBOL_RESOLVE);
        if (sym) {
            printf("[UPD] Set %s to %p\n", b->symbol, sym);
            *b->bind_target = sym;
        }
    }
}

void cdynsplit_reg (char* mpath, char* symbol, void** target, void* registrar) {
    printf("[DYN.REG] Registering m=%s : s=%s -> t=%p; registar=%p\n", mpath, symbol, target, registrar);
    Module* m = find_module(mpath);
    if (!m) {
        m = find_module(0);
        assert(m);
        strcpy(m->module_path, mpath);
        m->handle = 0;
        m->update_ts = 0;
        m->created_at = 0;
        printf("[DYN.REG] Added module '%s' at %i\n", m->module_path, calc_module_idx(m));
    }

    Binding* b = find_binding(mpath, symbol, target);
    if (!b) {
        b = acquire_binding();
        assert(b);
        b->module_idx = calc_module_idx(m);
        b->bind_target = target;
        b->registrar = registrar;
        if (dladdr(registrar, (Dl_info*)dl_info)) {
            Module* self = find_module(dl_info->mname);
            if (self) {
                int midx = calc_module_idx(self);
                b->registrar_module_idx = midx;
                printf("[DYN.REG] Found registrar to be %s midx=%i\n", self->module_path, midx);
            } else {
                printf("[DYN.REG] Couldn't find registrar (%p) module\n", registrar);
            }
        } else {
            printf("[DYN.REG.dladdr.fail] Couldn't dladdr\n");
        }
        strcpy(b->symbol, symbol);
        printf("[DYN.REG] Added entry for '%s' idx=%i ptr_at=%p\n", b->symbol, calc_binding_idx(b), b->bind_target);
    }

    update_needed = 1;
    cdynsplit_update();
}

#else
void cdynsplit_update () {}
void cdynsplit_reg (char* mpath, char* symbol, void** target) {}
#endif
