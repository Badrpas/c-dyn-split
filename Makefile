rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))


OUTDIR := out

all_src := $(call rwildcard,src,*.c)
dyn_src := $(call rwildcard,src/systems,*.c)

dyn_sofiles = $(dyn_src:src/%.c=${OUTDIR}/%.so)
dyn_objfiles = $(dyn_src:src/%.c=${OUTDIR}/%.o)
dyn_deps := $(dyn_sofiles:%.so=%.d)

host_src := ${filter-out $(dyn_src),${all_src}}
host_objfiles = $(host_src:src/%.c=${OUTDIR}/%.o)
host_deps := $(host_sofiles:%.so=%.d)

HOST_FILEPATH := ${OUTDIR}/main.host

# .PHONY: build_split build_sofiles default info

default: build_split info

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


build_unified:
	@echo Unified build not implemented

build_split: ${HOST_FILEPATH} build_sofiles

${HOST_FILEPATH}: ${host_objfiles}
	@mkdir -p $(@D)
	cc -o ${HOST_FILEPATH} $^ -rdynamic

${host_objfiles}: ${OUTDIR}/%.o: src/%.c
	@mkdir -p $(@D)
	cc -o ${@} -c $< -MMD -MP



build_sofiles: ${dyn_sofiles}

${dyn_sofiles}: %.so: %.o
	@mkdir -p $(@D)
	cc -o ${@} $^  -shared -rdynamic


${dyn_objfiles}: ${OUTDIR}/%.o: src/%.c
	@mkdir -p $(@D)
	cc -o ${@} -c $< -MMD -MP -fPIC


-include $(dyn_deps)
-include $(host_deps)

clean:
	@rm -rf ${OUTDIR}


