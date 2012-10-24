# Project name
PROJECT=tcpstreamer

# Build directories
OBJ_OUT=build
EXT_OUT=streamers

# Shared library functions
INC_DIR=lib

# Source code directories
SRC_DIR=src/core 
EXT_DIR=src/streamers

# Custom defines and constants
INCLUDE=lib/defines.h
DEFINES=ANSI DEBUG



### Compiler and linker settings ###
CC := colorgcc
LD := gcc
CFLAGS := -std=gnu99 -Wall -Wextra -pedantic -g
LDLIBS := pcap pthread rt


### Generic make variables ###
DEF := $(DEFINES:-D%=%) __STREAMER_DIR__="$(EXT_OUT)"
HDR := $(INCLUDE)
INC := $(INC_DIR:%/=%)
SRC := $(shell find $(SRC_DIR:%/=%)/ -type f -regex ".+\.c")
DIR := $(foreach d,$(EXT_DIR:%/=%),$(shell find $(d)/ -type d))
ALL := $(foreach d,$(SRC_DIR:%/=%) $(DIR:%/=%) $(INC),$(shell find $(d)/ -type f -regex ".+\.[ch]")) Makefile README.md filter.sh 

OBJ_OUT := $(firstword $(OBJ_OUT:%/=%))
EXT_OUT := $(firstword $(EXT_OUT:%/=%))


### Helper functions ###
expand_files = $(shell find $(1) -mindepth 2 -type f -regex ".+\.c")
target_name = $(subst /,_,$(patsubst ./%,%,$(1)))


### Make targets ###
.PHONY: $(PROJECT) all clean realclean streamers tar todo
all: $(PROJECT) streamers 

define compile_target_tmpl
$(OBJ_OUT)/$(2): $(1) $(HDR)
	-@mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) -fPIC $(addprefix -I,$(INC)) $(addprefix -include,$(HDR)) $(addprefix -D,$(DEF)) -o $$@ -c $$<
$(3) += $(OBJ_OUT)/$(2)
endef

define link_target_tmpl
$(EXT_OUT)/$(1): $$($(2))
	-@mkdir -p $$(@D)
	$$(LD) -fPIC -shared -nostartfiles $$(addprefix -l,$$(LDLIBS:-l%=%)) -o $$@ $$^
EXT += $(EXT_OUT)/$(1)
endef

$(foreach file,$(SRC),$(eval $(call compile_target_tmpl,$(file),$(call target_name,$(file:%.c=%.o)),OBJ)))
$(foreach d,$(DIR),$(foreach file,$(call expand_files,$(d)),$(eval $(call compile_target_tmpl,$(file),$(call target_name,$(file:%.c=%.so)),$(call target_name,$(d))_OBJ))))
$(foreach d,$(DIR),$(eval $(call link_target_tmpl,test,$(call target_name,$(d))_OBJ)))

$(PROJECT): $(OBJ)
	$(LD) -rdynamic -ldl $(addprefix -l,$(LDLIBS:-l%=%)) -o $@ $^

streamers: $(EXT)

clean:
	-$(RM) $(OBJ) $(EXT)

realclean: clean
	-$(RM) $(PROJECT)

tar: $(ALL)
	-ln -sf ./ $(PROJECT)
	tar czf $(PROJECT)-$(shell date +%Y%m%d%H%M).tar.gz $(patsubst %,$(PROJECT)/%,$^)
	-$(RM) $(PROJECT)

todo:
	-@for file in $(ALL:Makefile=); do \
		fgrep -H -e TODO -e FIXME $$file; \
	done; true

