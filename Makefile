#Project name
PROJECT=streamer

# Temporary directory to store object files
TMP_DIR=build

# Core source code directory
SRC_DIR=core

# Custom streamers source code
AUX_DIR=src

# Global headers
LIB_DIR=lib

# Custom defines and constants
DEFINES=DEF_SIZE=1460 DEF_PORT=50000 DEF_DUR=10



### Generic make variables ###
OUT := $(TMP_DIR:%/=%)
DIR := $(SRC_DIR:%/=%) $(AUX_DIR:%/=%)
LIB := $(LIB_DIR:%/=%)
SRC := $(shell find $(firstword $(DIR))/ -name "*.c") 
AUX := $(shell find $(lastword $(DIR))/ -maxdepth 1 -name "*.c")
HDR := $(foreach d,$(DIR) $(LIB),$(shell find $(d) -name "*.h"))
ALL := $(SRC) $(HDR) $(AUX) Makefile README.md autogen.sh
DEF := $(DEFINES:-D%=%)

### Compiler and linker settings ###
CC := colorgcc # gcc
LD := gcc
CFLAGS := -std=gnu99 -Wall -Wextra -pedantic -g 
LDLIBS := pcap pthread


### Make targets ###
.PHONY: $(PROJECT) all deploy clean todo
all: $(PROJECT)

define target_template
$(OUT)/$(if $(3),$(3)_)$(subst /,_,$(patsubst ./%,%,$(dir $(1:$(2)/%=%))))$(notdir $(1:%.c=%.o)): $(1)
	-@mkdir -p $$(@D)
	$$(CC) $(if $(4),$(addprefix -I,$(4))) $$(CFLAGS) $(addprefix -D,$(DEF:-D%=%)) -o $$@ -c $$?
OBJ+=$(OUT)/$(if $(3),$(3)_)$(subst /,_,$(patsubst ./%,%,$(dir $(1:$(2)/%=%))))$(notdir $(1:%.c=%.o))
ifeq (1,$(5))
DEP+=$(1)
endif
endef
$(foreach file,$(SRC),$(eval $(call target_template,$(file),$(firstword $(DIR)),,$(LIB))))
$(foreach file,$(AUX),$(eval $(call target_template,$(file),$(lastword $(DIR)),streamer,$(firstword $(DIR)) $(LIB),1)))

$(OUT)/$(PROJECT).o: 
	./autogen.sh $(OUT)/autogen.c entry_points $(DEP)
	$(CC) $(addprefix -include,$(shell find $(LIB) -name "*.h")) -c -o $@ $(OUT)/autogen.c


$(PROJECT): $(OBJ) $(OUT)/$(PROJECT).o
	$(LD) $(addprefix -l,$(LDLIBS:-l%=%)) -o $@ $^

deploy: $(ALL)
	-ln -sf ./ $(PROJECT)
	tar czf $(PROJECT)-$(shell date +%Y%m%d%H%M).tar.gz $(patsubst %,$(PROJECT)/%,$^)
	-$(RM) $(PROJECT)

clean:
	-$(RM) $(OBJ) $(OUT)/$(PROJECT).o $(OUT)/autogen.c $(PROJECT)

todo:
	-@for file in $(ALL:Makefile=); do \
		fgrep -H -e TODO -e FIXME $$file; \
	done; true
