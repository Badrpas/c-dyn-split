# C Split

Makefile-based template to build C code with live reload.

### Quickstart

Oneliner to init everything:

```shell
curl https://raw.githubusercontent.com/badrpas/c-dyn-split/master/Makefile > Makefile && make init
```

Then build
```shell
make
```

This will produce `out/main.host` executable and `out/sub.dyn.so`. Run the `out/main.host` and keep it running.

Update `src/sub.dyn.c`, rebuild with `make` and observe the change applied.

Done.

### Example

```c
// hello.dyn.c
// The .dyn suffix marks it as a reloadable

#include <stdio.h>

void say_hello() {
    printf("Hello from lib\n");
}
```

```c
// main.c

// This "declares" say_hello() function.
// When built as `make split`, it actually implements a proxy function(s).
// But does declarations otherwise for `make unified` build and proper LSP/intellisense.
#include "hello.dyn.gen.h" 

extern unsigned int sleep (unsigned int __seconds); // or `#include <unistd.h>`

int main () {
    while (1) {
        cdynsplit_update(); // Load/update the hello.dyn.c code
        say_hello();        // Call the "remote" function in regular way
        sleep(1);           // To not overspam it
    }
    return 0;
}
```

Build by running `make split`. This will produce `out/hello.dyn.so` lib and `out/main.host` executable.

Run it to get endless loop of hello messages.

Change the message in say_hello. Execute `make split` again (while `out/main.host` is still running).

??? Profit


### How it works

Makefile builds `%.dyn.c` files as dynamic libraries (`.so`) and generates headers files for them (`.dyn.gen.h`).

From calling side (host) it is as easy as to include the header and call the function as you would normally.

Headers include implementation proxy function that contains a pointer to remote (`.so`) counterpart function.

Host should call `upd_dyn()` to load libs and/or poll for their changes. It also resolves pointers in proxy functions to latest version.


Currenlty function proxies are generated via js script and `make` will install [`bun`](https://bun.sh) runtime if not found with `which bun`.


#### todo

- migrate codegen to c
- encapsulate everything in one dir
- better initial setup experience (curl? single Makefile download?)

