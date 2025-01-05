#include "stdio.h"
#include <assert.h>
#include <dlfcn.h>
#include <linux/limits.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DYN(path, symbol, ret)                         \
    ret {                                              \
        void* handle = dlopen(path, RTLD_NOW);         \
        if (!handle)                                   \
            return;                                    \
        typeof(symbol)* func = dlsym(handle, #symbol); \
        if (func) {                                    \
            func(0, 0);                                \
        } else {                                       \
            printf("no kek\n");                        \
        }                                              \
        dlclose(handle);                               \
    }

void hosted () { printf("Hosted Functioon\n"); }
void hosted2 () { printf("Hosted Functioon2\n"); }
void hosted3 () { printf("Hosted Functioon 3\n"); }

void do_kek_proto (int a, int b) {
    void* kek_handle = dlopen("systems/kek.so", RTLD_NOW);
    printf("handle %p\n", kek_handle);
    if (!kek_handle) {
        printf("no handle\n");
        return;
    }
    typeof(do_kek_proto)* func = dlsym(kek_handle, "do_kek");
    /* void (*func)(int a, int b) = dlsym(kek_handle, "do_kek"); */
    printf("handle %p\n", do_kek_proto);

    if (func) {
        func(a, b);
    } else {
        printf("no kek\n");
    }
    dlclose(kek_handle);
}

#define DEF(decl) typeof(decl)* fn

struct {
    struct {
        char* module_path;
        char* symbol;
        void** bind_target;
    } entries[100];
    struct {
        char* module_path;
        void* handle;
        time_t loaded_ts;
    } modules[100];
} dyn_handler = {0};

void upd_dyn () {
    time_t now = time(0);
    for (int module_idx = 0; module_idx < 100; module_idx++) {
        typeof(dyn_handler.modules[module_idx])* m = &dyn_handler.modules[module_idx];
        if (!m->module_path) {
            continue;
        }
        double diff_seconds = difftime(now, m->loaded_ts);
        if (diff_seconds < 5.) {
            continue;
        }
        printf("Updating %.2f %s\n", diff_seconds, m->module_path);

        m->loaded_ts = now;
        if (m->handle) {
            printf("closing %p\n", m->handle);
            dlclose(m->handle);
        }

        printf("opening %s\n", m->module_path);
        m->handle = dlopen(m->module_path, RTLD_NOW | RTLD_GLOBAL);

        for (int idx = 0; idx < 100; idx++) {
            typeof(dyn_handler.entries[idx])* entry = &dyn_handler.entries[idx];
            if (!entry->module_path)
                continue;
            printf("strcmp with %s\n", entry->module_path);
            if (!strcmp(m->module_path, entry->module_path)) {
                printf("dlsym %p %s\n", m->handle, entry->symbol);
                assert(entry->bind_target);
                *entry->bind_target = dlsym(m->handle, entry->symbol);
            }
        }
    }
}

void reg_dyn (char* mpath, char* symbol, void** target) {
    for (int i = 0; i < 100; i++) {
        typeof(dyn_handler.entries[i])* entry = &dyn_handler.entries[i];
        if (entry->module_path) {
            if (!strcmp(entry->module_path, mpath)) {
                if (!strcmp(entry->symbol, symbol)) {
                    if (entry->bind_target != target) {
                        return;
                    }
                }
            }
            continue;
        }
        entry->module_path = mpath;
        entry->symbol = symbol;
        entry->bind_target = target;
        for (int j = 0; j < 100; j++) {
            if (dyn_handler.modules[j].module_path) {
                if (!strcmp(dyn_handler.modules[j].module_path, mpath)) {
                    return;
                }
                continue;
            }
            dyn_handler.modules[j].module_path = mpath;
            upd_dyn();
            break;
        }
        return;
    }
    // no free estate
    assert(0);
}

#define REG_DYN(module, symbol) reg_dyn(module, #symbol, (void**)(&symbol))
#define CALL_DYN(ident, ...) \
    if (ident)               \
    ident(__VA_ARGS__)

void (*do_kek)(int a, int b) = 0;

int main (int argc, char** argv) {
    reg_dyn("systems/kek.so", "do_kek", (void**)(&do_kek));

    while (1) {
        upd_dyn();
        CALL_DYN(do_kek, 4, 5);

        sleep(2);
    }

    return 0;
}
