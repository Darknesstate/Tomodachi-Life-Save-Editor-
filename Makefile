TARGET := TomodachiReal
BUILD := build
SOURCES := source
INCLUDES := include
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -O2 -mword-relocations -fomit-frame-pointer -ffunction-sections $(ARCH) -DARM11 -D_3DS
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lcitro2d -lcitro3d -lctru -lm
include $(DEVKITARM)/3ds_rules
