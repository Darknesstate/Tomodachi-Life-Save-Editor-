TARGET := TomodachiLifeSaveEditor
BUILD := build
SOURCES := source
INCLUDES := include
DATA := data
APP_TITLE := Tomodachi Life Save Editor
APP_DESCRIPTION := Dinheiro Max no Tomodachi Life
APP_AUTHOR := Darkness Metal
APP_ICON := icon.png
ARCH := arm-none-eabi
PREFIX := $(DEVKITARM)/bin/$(ARCH)-
CC := $(PREFIX)gcc
CXX := $(PREFIX)g++
LD := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
CFLAGS := -g -O2 -w -mword-relocations -ffunction-sections -fdata-sections
CFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS += -Wno-error -Wno-format -Wno-int-conversion -Wno-misleading-indentation -Wno-unused-parameter -Wno-unused-variable
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS := -g $(ARCHFLAGS)
LDFLAGS := -specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -Wl,-Map,$(notdir $*.map)
LIBS := -lctru -lm
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
OFILES := $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(SFILES:.s=.o)
OFILES := $(addprefix $(BUILD)/,$(OFILES))
all: $(TARGET).smdh $(TARGET).3dsx
$(BUILD):
	@mkdir -p $@
$(BUILD)/%.o: $(SOURCES)/%.c | $(BUILD)
	$(CC) -MMD -MP -MF $(@:.o=.d) $(CFLAGS) -I$(INCLUDES) -c $< -o $@
$(BUILD)/%.o: $(SOURCES)/%.cpp | $(BUILD)
	$(CXX) -MMD -MP -MF $(@:.o=.d) $(CXXFLAGS) -I$(INCLUDES) -c $< -o $@
$(BUILD)/%.o: $(SOURCES)/%.s | $(BUILD)
	$(CC) -MMD -MP -MF $(@:.o=.d) $(ASFLAGS) -c $< -o $@
$(TARGET).elf: $(OFILES)
	$(LD) $(LDFLAGS) $(OFILES) $(LIBS) -o $@
$(TARGET).smdh: $(APP_ICON)
	@smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(APP_ICON) $(TARGET).smdh || smdhtool --create "Tomodachi Life Save Editor" "Dinheiro Max" "Darkness Metal" icon.png $(TARGET).smdh
$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool --smdh=$(TARGET).smdh --romfs=$(DATA) $< $@
clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh
