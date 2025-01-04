rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

sources := $(call rwildcard,src/systems,*.c)
sofiles = $(sources:src/%.c=out/%.so)

all: build_sofiles
	@date
	@date +%s

complete:

build_sofiles: ${sofiles}

${sofiles}: out/%.so: src/%.c
	@mkdir -p $(@D)
	cc $^ -o ${@} -fPIC -shared 


clean:
	@rm -rf out/


