rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))


SRCDIR := src
OUTDIR := out
BUILDIR := build

HOST_CFLAGS := ${CFLAGS}
DYN_CFLAGS := ${CFLAGS}

all_src := $(call rwildcard,${SRCDIR},*.c)
dyn_src := $(call rwildcard,${SRCDIR},*.dyn.c)
# gen_impl_src := $(call rwildcard,${SRCDIR},*.dyn.impl.gen.c)

generated_dyn_headers := $(call rwildcard,${SRCDIR},*.dyn.gen.h)

dyn_sofiles = $(dyn_src:${SRCDIR}/%.c=${OUTDIR}/%.so)
dyn_objfiles = $(dyn_src:${SRCDIR}/%.c=${BUILDIR}/%.o)
dyn_deps := $(dyn_objfiles:%.o=%.d)
dyn_headerfiles := $(dyn_src:%.dyn.c=%.dyn.gen.h)

# unified_src := ${filter-out $(gen_impl_src),${all_src}}
unified_src := ${all_src}
unified_objfiles := ${unified_src:${SRCDIR}/%.c=${BUILDIR}/%.o}
unified_deps := $(unified_objfiles:%.o=%.d)

host_src := ${filter-out $(dyn_src),${all_src}}
# host_src := ${filter-out $(gen_impl_src),${host_src}}
host_objfiles = $(host_src:${SRCDIR}/%.c=${BUILDIR}/%.o)
host_deps := $(host_objfiles:%.o=%.d)

host_executable := ${OUTDIR}/main.host

# .PHONY: build_split default info

# nm  --defined-only -f just-symbols

default: build_split

info:
	@echo
	@echo all: ${all_src}
	@echo host: ${host_src}
	@echo dyn: ${dyn_src}
	@echo dyn so: ${dyn_sofiles}
	@echo dyn deps: ${dyn_deps}
	@echo
	@date
	@date +%s


u: unified

unified: unified_executable
	@echo ${unified_src}
	@echo ${unified_objfiles}


${unified_objfiles}: ${dyn_headerfiles}

unified_executable: ${unified_objfiles}
	@mkdir -p $(OUTDIR)
	cc -o ${OUTDIR}/main.unified ${unified_objfiles}


build_split: HOST_CFLAGS := ${HOST_CFLAGS} -MMD -MP  -D_DYN_SPLIT_BUILD -g
build_split: DYN_CFLAGS := ${DYN_CFLAGS} -MMD -MP -fPIC -D_DYN_SPLIT_BUILD -g

build_split: ${dyn_sofiles} ${host_executable} 

build_sofiles: ${dyn_sofiles} 

${host_executable}: ${host_objfiles}
	@mkdir -p $(@D)
	cc -o ${host_executable} $^ -rdynamic

${host_objfiles}: ${BUILDIR}/%.o: ${SRCDIR}/%.c
	@mkdir -p $(@D)
	cc -o ${@} -c $< ${HOST_CFLAGS}


${dyn_sofiles}: ${OUTDIR}/%.so: ${BUILDIR}/%.o
	@mkdir -p $(@D)
	cc -o ${@} $^  -shared -rdynamic

${dyn_objfiles}: ${BUILDIR}/%.dyn.o: ${SRCDIR}/%.dyn.gen.h

self_guard = $(shell echo "${<:$(SRCDIR)/%.c=%.c}" | sed -e 's#\([\/_\.-]\)#_#g' | sed -e 's#\(.*\)#\U\1#g')

${dyn_objfiles}: ${BUILDIR}/%.o: ${SRCDIR}/%.c
	@mkdir -p $(@D)
	cc -o ${@} -c $< ${DYN_CFLAGS} -D$(self_guard)
	@mv ${@:%.o=%.d} ${@:%.o=%.t}
	awk -f ./awkrecipe ${@:%.o=%.t} > ${@:%.o=%.d}
	@rm ${@:%.o=%.t}

%.dyn.gen.h: %.dyn.c
	bun run ./gen/scan.js `realpath $<`

-include $(dyn_deps)
-include $(host_deps)
-include $(unified_deps)

clean:
	rm -rf ${BUILDIR}
	rm -rf ${OUTDIR}
	rm ${generated_dyn_headers}


