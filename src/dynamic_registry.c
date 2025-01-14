#include <unistd.h>
#define _DYN_SPLIT_BUILD

#ifdef _DYN_SPLIT_BUILD

#include "stdio.h"
#include <assert.h>
#include <dlfcn.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define UPDATE_INTERVAL_SECONDS 1.0

#define MAX_MODULES 100
#define MAX_BINDINGS 100
#define MAX_NAME_LEN 255

typedef struct {
    int module_idx;
    char symbol[MAX_NAME_LEN + 1];
    void** bind_target;
} Binding;
typedef struct {
    char module_path[MAX_NAME_LEN + 1];
    void* handle;
    time_t update_ts;
    time_t created_at;
} Module;

typedef struct {
    Binding entries[MAX_BINDINGS];
    Module modules[MAX_MODULES];
} DYN_HANDLER;

DYN_HANDLER dyn_handler = {0};

static Module* find_module (char* m_name) {
    for (int i = 1; i < MAX_MODULES; i++) {
        if (m_name && !strcmp(dyn_handler.modules[i].module_path, m_name) ||
            (!m_name && !dyn_handler.modules[i].handle)) {
            return &dyn_handler.modules[i];
        }
    }
    return 0;
}

static Binding* find_binding (char* m_name, char* e_symbol, void** target) {
    for (int i = 1; i < MAX_BINDINGS; i++) {
        Binding* b = &dyn_handler.entries[i];
        if (e_symbol && !strcmp(b->symbol, e_symbol) && m_name && b->module_idx &&
            !strcmp(dyn_handler.modules[b->module_idx].module_path, m_name) && target && target == b->bind_target) {
            return b;
        }
    }
    return 0;
}
static Binding* acquire_binding () {
    for (int i = 1; i < MAX_BINDINGS; i++) {
        Binding* b = &dyn_handler.entries[i];
        if (!b->module_idx)
            return b;
    }
    assert(0);
    return 0;
}

static int calc_module_idx (Module* m) {
    return ((long long)(void*)m - (long long)(void*)&dyn_handler.modules) / sizeof(*m);
}
static int calc_binding_idx (Binding* b) {
    return ((long long)(void*)b - (long long)(void*)&dyn_handler.entries) / sizeof(*b);
}

struct {
    const char* dli_fname; /* Pathname of shared object that
                              contains address */
    void* dli_fbase;       /* Address at which shared object
                              is loaded */
    const char* dli_sname; /* Name of nearest symbol with address
                              lower than addr */
    void* dli_saddr;       /* Exact address of symbol named
                              in dli_sname */
} __dl_info = {0};

static char* err = 0;
#define CHECK_ERR(stage)                                        \
    do {                                                        \
        err = dlerror();                                        \
        if (err)                                                \
            printf("[DL_ERROR] while [" #stage "]: %s\n", err); \
    } while (0)

static int update_needed = 0;

void upd_dyn () {
    time_t now = time(0);
    for (int module_idx = 1; module_idx < MAX_MODULES; module_idx++) {
        Module* m = &dyn_handler.modules[module_idx];
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
            /* printf("[UPD] closing %p\n", m->handle); */
            dlclose(m->handle);
            CHECK_ERR(CLOSING);
        }

        m->handle = dlopen(m->module_path, RTLD_LAZY);
        CHECK_ERR(OPENING);
        if (prev_handle != m->handle) {
            printf("[UPD] handle changed %p -> %p\n", prev_handle, m->handle);
            /* } else { */
            /* printf("[UPD] handle unchanged\n"); */
        }
        update_needed = 1;
    }

    if (!update_needed) {
        printf("[UPD] Nothing changed\n");
        return;
    } else {
        printf("[UPD] Changes occured\n");
    };

    update_needed = 0;

    for (int i = 1; i < MAX_BINDINGS; i++) {
        Binding b = dyn_handler.entries[i];
        if (!b.module_idx)
            continue;
        Module m = dyn_handler.modules[b.module_idx];
        void* sym = dlsym(m.handle, b.symbol);
        CHECK_ERR(SYMBOL_RESOLVE);
        if (sym) {
            printf("[UPD] Set %s to %p\n", b.symbol, sym);
            *b.bind_target = sym;
        }
    }
}

void reg_dyn (char* mpath, char* symbol, void** target) {
    printf("[DYN.REG] Registering %s : %s -> %p\n", mpath, symbol, target);
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
        strcpy(b->symbol, symbol);
        printf("[DYN.REG] Added entry for '%s' idx=%i ptr_at=%p\n", b->symbol, calc_binding_idx(b), b->bind_target);
    }

    update_needed = 1;
    upd_dyn();
}

#else
void upd_dyn () {}
void reg_dyn (char* mpath, char* symbol, void** target) {}
#endif
