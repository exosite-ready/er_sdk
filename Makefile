#This is just an Example on how to build the ERSDK examples with gcc on a POSIX system
#The ERSDK can be built with various compilers and various compiler options

#Default compiler for examples is GCC
CPP=gcc
CC=gcc
OBJCOPY=objcopy

#BUILD flags for GCC on x86
ifeq ($(shell uname), Linux)
LDGROUP_START= -Wl,--start-group 
LDGROUP_END= -Wl,--end-group
LDFLAGS += -Wl,--gc-sections -m32 -lrt -lpthread -lm
CFLAGS += -D_POSIX_C_SOURCE=200809L
else
LDGROUP_START= 
LDGROUP_END=
LDFLAGS += -m32
endif
CFLAGS += -g -m32 -ffunction-sections -fdata-sections -fno-common -fno-strict-aliasing -fomit-frame-pointer
CFLAGS += -std=c99 -Werror -Wall -Wmissing-prototypes
CFLAGS_EXTRA += -Wextra -Wwrite-strings -Wcast-qual -Wcast-align -Wshadow -Wstrict-prototypes -Wold-style-definition -Wunused-result
CFLAGS += $(ER_SDK_FLAG) $(ER_SDK_INC)

#Set make parameters
TARGET_NAME=ersdk_test.elf
EXAMPLE=examples/exosite_send_http.o
$(TARGET_NAME)_LIBRARIES += $(ER_LIBRARIES)
BUILDDIR = .build/$(TARGET_NAME)
DEPDIR = .build/$(TARGET_NAME)

ifneq ($(MAKECMDGOALS),clean)
$(info Configuring sources (this may take ~10 sec) ...)
$(eval m=$(shell ./configure.sh -foil -c false > modules.mk))
endif

include modules.mk
#=============================================================================
# Build Rules
# - .c and .s rules generate dependency rules automatically
# - static library rules are generated based on $(TARGET)_LIBRARIES variable
#   $(TARGET)_LIBRARIES has to be set in modules.mk, it is a list of libraries
#   the name has to be in a format: <name>.a
#   libraryname_OBJS has to be filled, this specifies the objects for a 
#   static library
# - exostat generates size statistics for the static libraries: 
#   library name: <code size>, <data size>
#=============================================================================
$(BUILDDIR)/%.o : %.c
	@mkdir -p $(dir $@)
	$(eval df := $(patsubst %,$(DEPDIR)/%,$*))
	@$(CPP) -MM $(CFLAGS) $< -o $(df).d -MT $@;\
          cp $(df).d $(df).P; \
          sed -e 's,#.*,,' -e 's,^[^:]*: *,,' -e 's, *\\$$,,' \
              -e '/^$$/d' -e 's,$$, :,' < $(df).d >> $(df).P; \
          rm -f $(df).d

	$(eval RELATIVE_OBJ_NAME=$(<:%.c=%.o))
	$(info [CC] $<)
	$(if $(findstring er_sdk, $<), $(eval CFLAGSE=$(CFLAGS_EXTRA)))
	$(if $(findstring er_sdk/third_party, $<), $(eval CFLAGSE=))
	$(if $(findstring er_sdk/porting, $<), $(eval CFLAGSE=))

	@$(CC) $(CFLAGSE) $(CFLAGS) $($(RELATIVE_OBJ_NAME)_FLAGS) -c -o $@ $<
    
$(BUILDDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(eval df := $(patsubst %,$(DEPDIR)/%,$*))
	@$(CPP) -MM $(CFLAGS) $< -o $(df).d -MT $*.o;\
          cp $(df).d $(df).P; \
          sed -e 's,#.*,,' -e 's,^[^:]*: *,,' -e 's, *\\$$,,' \
              -e '/^$$/d' -e 's,$$, :,' < $(df).d >> $(df).P; \
          rm -f $(df).d
	$(info [AS] $<)          
	@$(CC) $(ASFLAGS) -o $@ $<

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(eval df := $(patsubst %,$(DEPDIR)/%,$*))
	@$(CPP) -MM $(CFLAGS) $< -o $(df).d -MT $*.o;\
          cp $(df).d $(df).P; \
          sed -e 's,#.*,,' -e 's,^[^:]*: *,,' -e 's, *\\$$,,' \
              -e '/^$$/d' -e 's,$$, :,' < $(df).d >> $(df).P; \
          rm -f $(df).d
	$(info [AS] $<)
	@$(CC) $(ASFLAGS) -o $@ $<

all: $(TARGET_NAME) 

.PHONY: clean
clean:
	find . -name "*.elf" | xargs rm -rf
	rm -rf .build

define LIBRARY_template
$(BUILDDIR)/lib$(1): $$($(1)_TOBJS)
	@$(AR) -crs $(BUILDDIR)/lib$(1) $$($(1)_TOBJS)
	$$(info [Archiving] lib$(1)) 
endef

define OBJECT_template
$(1)_TOBJS = $(patsubst %, $(BUILDDIR)/%, $($(1)_OBJS))
$(eval ALL_DOBJS += $$($(1)_OBJS))
endef

#$(eval $(info Flag $(A)_FLAGS : $($(A)_FLAGS)))
define flag_template
$(eval A=$(findstring  $(2), $($(1)_OBJS)))
$(if $(A), $(eval $(A)_FLAGS=$($(1)_FLAGS)))
endef

#Generate real library names
$(TARGET_NAME)_OBJS += $(EXAMPLE)
$(TARGET_NAME)_REAL_LIBNAMES = $(patsubst %, $(BUILDDIR)/lib%, $($(TARGET_NAME)_LIBRARIES))
$(TARGET_NAME)_REAL_OBJNAMES = $(patsubst %, $(BUILDDIR)/%, $($(TARGET_NAME)_OBJS))

#Generate object list for a library name
$(foreach prog,$($(TARGET_NAME)_LIBRARIES),$(eval $(call OBJECT_template,$(prog))))

#Generate Library rules
$(foreach prog,$($(TARGET_NAME)_LIBRARIES), $(eval $(call LIBRARY_template,$(prog))))

#Generate module specific flag variable for all object; the flags are set based on module specific flags
$(foreach obj,$(ALL_DOBJS), $(foreach prog,$($(TARGET_NAME)_LIBRARIES), $(eval $(call flag_template,$(prog), $(obj)))) )

$(TARGET_NAME): $($(TARGET_NAME)_REAL_LIBNAMES) $($(TARGET_NAME)_REAL_OBJNAMES)
	$(CC) $(NEWLIB_STUB) $($(TARGET_NAME)_REAL_OBJNAMES) -L./$(BUILDDIR) $(LDGROUP_START) $($(TARGET_NAME)_LIBRARIES:%.a=-l%) $(LDFLAGS) $(LDGROUP_END) -o $(TARGET_NAME)
	$(info [LINKING] $@)

ALL_SRC := $(ALL_DOBJS:.o=.c)
-include $(ALL_SRC:%.c=$(DEPDIR)/%.P)
