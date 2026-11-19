#[STRIPPED 81 bytes]
.SUFFIXES:
#[STRIPPED 81 bytes]
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#[STRIPPED 81 bytes]
TARGET := TomodachiLifeSaveEditor
BUILD := build
SOURCES := source
DATA := data
INCLUDES := include

APP_TITLE := Tomodachi Life Save Editor
APP_DESCRIPTION := Dinheiro Max no Tomodachi Life
APP_AUTHOR := Darkness Metal
ICON := icon.png

#[STRIPPED 81 bytes]
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -O2 -w -mword-relocations -ffunction-sections -fdata-sections $(ARCH)
CFLAGS += -Wno-error -Wno-format -Wno-int-conversion -Wno-misleading-indentation -Wno-unused-parameter -Wno-unused-variable
CFLAGS += $(INCLUDE) -D__3DS__ -DARM11 -D_3DS

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17

ASFLAGS := -g $(ARCH)

LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lctru -lm

#[STRIPPED 81 bytes]
LIBDIRS := $(PORTLIBS) $(CTRULIB)
#[STRIPPED 81 bytes]
ifneq ($(BUILD),$(notdir $(CURDIR)))
#[STRIPPED 81 bytes]
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXDEPS := $(if $(NO_SMDH),,$(OUTPUT).smdh)

ifeq ($(strip $(ICON)),)
icons := $(wildcard *.png)
ifneq (,$(findstring $(TARGET).png,$(icons)))
export APP_ICON := $(TOPDIR)/$(TARGET).png
else
ifneq (,$(findstring icon.png,$(icons)))
export APP_ICON := $(TOPDIR)/icon.png
endif
endif
else
export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

.PHONY: all clean
#[STRIPPED 81 bytes]
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

#[STRIPPED 81 bytes]
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf

#[STRIPPED 81 bytes]
else
#[STRIPPED 81 bytes]
$(OUTPUT).3dsx : $(OUTPUT).elf $(_3DSXDEPS)
$(OFILES_SOURCES) : $(HFILES)
$(OUTPUT).elf : $(OFILES)
%.bin.o %_bin.h : %.bin
	@echo $(notdir $<)
	@$(bin2o)
-include $(DEPSDIR)/*.d
#[STRIPPED 87 bytes]
endif
#[STRIPPED 87 bytes]
