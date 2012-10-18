#Project name
PROJECT=streamer

# Temporary directory to store object files
TMP_DIR=build

# Exported headers and shared library function declarations
INC_DIR=lib

# Source code directory
SRC_DIR=src

# Custom streamers directory
EXT_DIR=streamers

# Custom defines and constants
INCLUDE=lib/defines.h
DEFINES=ANSI



### Generic make variables ###
DEF := $(DEFINES:-D%=%)
OUT := $(TMP_DIR:%/=%)
INC := $(INC_DIR:%/=%)
SRC := $(foreach d,$(SRC_DIR:%/=%),$(shell find $(d)/ -name "*.c"))
EXT := $(foreach d,$(EXT_DIR:%/=%),$(shell find $(d)/ -name "*.c"))
HDR := $(foreach d,$(SRC_DIR:%/=%) $(INC) $(EXT_DIR:%/=%),$(shell find $(d)/ -name "*.h"))
ALL := $(SRC) $(HDR) Makefile README.md autogen.sh


### Compiler and linker settings ###
CC := colorgcc # gcc
LD := gcc
CFLAGS := -std=gnu99 -Wall -Wextra -pedantic -g 
LDLIBS := pcap pthread


### Templates for auto-generated code ###
define struct_tmpl
struct streamer { \
	const char *id; \
        void (*bootstrapper)(); \
};
endef

define fundecl_tmpl 
extern void __$(1)();
endef

define arraydecl_tmpl 
struct streamer __streamers[] = { $(foreach fun,$(FUN),{"$(fun)", &__$(fun)},) {0, 0} };
endef


### Make targets ###
.PHONY: $(PROJECT) all clean realclean tar todo
all: $(PROJECT)

define target_template
$(OUT)/$(2): $(1) $(INCLUDE)
	-@mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) $(addprefix -I, $(INC)) $(addprefix -include, $(INCLUDE)) $(addprefix -D,$(DEF)) -o $$@ -c $$<
OBJ += $(OUT)/$(2)
GEN += $(if $(3),$(OUT)/$(2))
FUN += $(if $(3),$(shell sed -nE $(3) $(1)))
endef
$(foreach file,$(SRC),$(eval $(call target_template,$(file),$(subst /,_,$(patsubst ./%,%,$(file:%.c=%.o))))))
$(foreach file,$(EXT),$(eval $(call target_template,$(file),$(subst /,_,$(patsubst ./%,%,$(file:%.c=%.o))),"s/\s*STREAMER\(\s*&?(\w+)\s*(,\s*&?\w+)*\s*\)/\1/p")))

$(OUT)/autogen.o: $(GEN)
	@echo '$(call struct_tmpl)' > $(@:%.o=%.c)
	@echo '$(foreach fun,$(FUN),$(call fundecl_tmpl,$(fun)))' >> $(@:%.o=%.c)
	@echo '$(call arraydecl_tmpl)' >> $(@:%.o=%.c)
	$(CC) -c -o $@ $(@:%.o=%.c)

$(PROJECT): $(OBJ) $(OUT)/autogen.o
	$(LD) $(addprefix -l,$(LDLIBS:-l%=%)) -o $@ $^

clean:
	-$(RM) $(OBJ) $(OUT)/autogen.o 

realclean: clean
	-$(RM) $(OUT)/autogen.c $(PROJECT)

tar: $(ALL)
	-ln -sf ./ $(PROJECT)
	tar czf $(PROJECT)-$(shell date +%Y%m%d%H%M).tar.gz $(patsubst %,$(PROJECT)/%,$^)
	-$(RM) $(PROJECT)

todo:
	-@for file in $(ALL:Makefile=); do \
		fgrep -H -e TODO -e FIXME $$file; \
	done; true
