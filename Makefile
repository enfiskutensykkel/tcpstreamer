#Project name
PROJECT=streamer

# Temporary directory to store object files
# Must be a relative directory of the project directory
TMP_DIR=build

# Location of source code files
# Must be an existing subdirectory of project directory
SRC_DIR=src

# Custom defines and constants
DEFINES=DEF_SIZE=1460 DEF_PORT=50000 DEF_DUR=10 MAX_CONNS=10 \
    	ETH_FRAME_LEN=14 RTT_INTERVAL=1 MAX_INFLIGHT=3 



### Generic make variables ###
OUT := $(TMP_DIR)
SRC := $(shell find $(SRC_DIR)/ -mindepth 1 -name "*.c")
HDR := $(shell find $(SRC_DIR)/ -mindepth 1 -name "*.h")
ALL := $(SRC) $(HDR) Makefile README
DEF := $(DEFINES:-D%=%)

### Compiler and linker settings ###
CC := gcc
LD := gcc
CFLAGS := -Wall -Werror -g -std=gnu99
LDLIBS := pcap pthread


### Make targets ###
.PHONY: $(PROJECT) all deploy clean todo
all: $(PROJECT)

define target_template
$(OUT)/$(subst /,_,$(dir $(1:./%=%)))$(notdir $(1:%.c=%.o)): $(1)
	-@mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) $(addprefix -D,$(DEF:-D%=%)) -o $$@ -c $$?
OBJ+=$(OUT)/$(subst /,_,$(dir $(1:./%=%)))$(notdir $(1:%.c=%.o))
endef
$(foreach file,$(SRC),$(eval $(call target_template,$(file))))

$(PROJECT): $(OBJ)
	$(LD) $(addprefix -l,$(LDLIBS:-l%=%)) -emain -o $@ $^

deploy: $(ALL)
	-ln -sf ./ $(PROJECT)
	tar czf $(PROJECT)-$(shell date +%Y%m%d%H%M).tar.gz $(patsubst %,$(PROJECT)/%,$^)
	-$(RM) $(PROJECT)

clean:
	-$(RM) $(OBJ) $(PROJECT)

todo:
	-@for file in $(ALL:Makefile=); do \
		fgrep -H -e TODO -e FIXME $$file; \
	done; true
