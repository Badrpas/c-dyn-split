
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

C_DYN_DIR := .c-dyn-split

SRCDIR := src
OUTDIR := out
BUILDIR := build

CFLAGS += -Wall

HOST_CFLAGS := ${CFLAGS}
DYN_CFLAGS := ${CFLAGS}

all_src := $(call rwildcard,${SRCDIR},*.c)
dyn_src := $(call rwildcard,${SRCDIR},*.dyn.c)

generated_dyn_headers := $(call rwildcard,${SRCDIR},*.dyn.gen.h)

dyn_sofiles = $(dyn_src:${SRCDIR}/%.c=${OUTDIR}/%.so)
dyn_objfiles = $(dyn_src:${SRCDIR}/%.c=${BUILDIR}/%.o)
dyn_deps := $(dyn_objfiles:%.o=%.d)
dyn_headerfiles := $(dyn_src:%.dyn.c=%.dyn.gen.h)

unified_src := ${all_src}
unified_objfiles := ${unified_src:${SRCDIR}/%.c=${BUILDIR}/%.o}
unified_deps := $(unified_objfiles:%.o=%.d)

host_src := ${filter-out $(dyn_src),${all_src}}
host_objfiles = $(host_src:${SRCDIR}/%.c=${BUILDIR}/%.o)
host_deps := $(host_objfiles:%.o=%.d)

host_executable := ${OUTDIR}/main.host



default: split

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

unified: HOST_CFLAGS := ${HOST_CFLAGS} -MMD -MP
unified: DYN_CFLAGS := ${DYN_CFLAGS} -MMD -MP -fPIC

unified: unified_executable
	@echo
	@echo unified_src
	@echo ${unified_src}
	@echo unified_objfiles
	@echo ${unified_objfiles}
	@echo


${unified_objfiles}: ${dyn_headerfiles}

${OUTDIR}/main.unified: ${unified_objfiles} ${C_DYN_DIR}/dynamic_registry.o
	@mkdir -p $(OUTDIR)
	cc -o ${host_executable} $^

unified_executable: ${OUTDIR}/main.unified


split: HOST_CFLAGS := ${HOST_CFLAGS} -MMD -MP  -D_DYN_SPLIT_BUILD -g
split: DYN_CFLAGS := ${DYN_CFLAGS} -MMD -MP -fPIC -D_DYN_SPLIT_BUILD -g

split: ${dyn_sofiles} ${host_executable} 

build_sofiles: ${dyn_sofiles} 

${host_executable}: ${host_objfiles} ${C_DYN_DIR}/dynamic_registry.o
	@mkdir -p $(@D)
	cc -o ${host_executable} $^ -rdynamic

${host_objfiles}: ${BUILDIR}/%.o: ${SRCDIR}/%.c
	@mkdir -p $(@D)
	cc -o ${@} -c $< ${HOST_CFLAGS}


${C_DYN_DIR}/dynamic_registry.o: CFLAGS+=-fPIC -g -D_DYN_SPLIT_BUILD

${dyn_sofiles}: ${OUTDIR}/%.so: ${BUILDIR}/%.o
	@mkdir -p $(@D)
	cc -o ${@} $^  -shared -rdynamic

${dyn_objfiles}: ${BUILDIR}/%.dyn.o: ${SRCDIR}/%.dyn.gen.h

self_guard = $(shell echo "${<:$(SRCDIR)/%.c=%.c}" | sed -e 's#\([\/_\.-]\)#_#g' | sed -e 's#\(.*\)#\U\1#g')

${dyn_objfiles}: ${BUILDIR}/%.o: ${SRCDIR}/%.c
	@mkdir -p $(@D)
	cc -o ${@} -c $< ${DYN_CFLAGS} -D$(self_guard)
	@mv ${@:%.o=%.d} ${@:%.o=%.t}
	awk -f ./${C_DYN_DIR}/awkrecipe ${@:%.o=%.t} > ${@:%.o=%.d}
	@rm ${@:%.o=%.t}

%.dyn.gen.h: %.dyn.c ${C_DYN_DIR}/gen/node_modules/tree-sitter/index.js
	bun run ./${C_DYN_DIR}/gen/scan.js `realpath $<`

bunpath := $(shell which bun)
bunpath := $(if $(bunpath),$(bunpath),trigger_bun_install)

${C_DYN_DIR}/gen/node_modules/tree-sitter/index.js: $(bunpath)
	cd ${C_DYN_DIR}/gen && bun i

$(bunpath):
	curl -fsSL https://bun.sh/install | bash

-include $(dyn_deps)
-include $(host_deps)
-include $(unified_deps)

${C_DYN_DIR}/downloads/latest.tar.gz:
	mkdir -p ${C_DYN_DIR}/downloads
	curl https://codeload.github.com/badrpas/c-dyn-split/tar.gz/master > ${C_DYN_DIR}/downloads/latest.tar.gz


init: ${C_DYN_DIR}/downloads/latest.tar.gz .gitignore rtmuxer.yaml gdbrc ${SRCDIR}/main.c
	cd ${C_DYN_DIR} && tar -xzf ./downloads/latest.tar.gz --strip=2 c-dyn-split-master/.c-dyn-split
	cd ${C_DYN_DIR} && tar -xzf ./downloads/latest.tar.gz --strip=1 c-dyn-split-master/readme.md

update_self: ${C_DYN_DIR}/downloads/latest.tar.gz
	tar -xzf ./${C_DYN_DIR}/downloads/latest.tar.gz --strip=1 c-dyn-split-master/Makefile
	touch Makefile

.gitignore: ${C_DYN_DIR}/downloads/latest.tar.gz
	tar -xzf ./${C_DYN_DIR}/downloads/latest.tar.gz --strip=1 c-dyn-split-master/.gitignore
	touch $@

rtmuxer.yaml: ${C_DYN_DIR}/downloads/latest.tar.gz
	tar -xzf ./${C_DYN_DIR}/downloads/latest.tar.gz --strip=1 c-dyn-split-master/rtmuxer.yaml
	touch $@

gdbrc: ${C_DYN_DIR}/downloads/latest.tar.gz
	tar -xzf ./${C_DYN_DIR}/downloads/latest.tar.gz --strip=1 c-dyn-split-master/gdbrc
	touch $@

$(SRCDIR)/main.c: ${C_DYN_DIR}/downloads/latest.tar.gz
	@echo [!] No main.c file is found. Adding example files
	mkdir -p ${SRCDIR}
	cd ${SRCDIR} && tar -xzf ./${C_DYN_DIR}/downloads/latest.tar.gz --strip=3 c-dyn-split-master/examples/basic
	touch $@
	

clean:
	rm -rf ${BUILDIR} ${OUTDIR} ${generated_dyn_headers} ${C_DYN_DIR}/dynamic_registry.o ${C_DYN_DIR}/downloads



