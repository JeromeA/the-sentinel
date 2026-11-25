PYTHON ?= python3
TOOLCHAIN ?= tools/ia16
OUT ?= build
CFLAGS ?= -Os
ASSET_DIR ?= assets
ASSET_OUT = $(OUT)/assets
ASSET_INPUTS = $(ASSET_DIR)/images $(ASSET_DIR)/models $(wildcard $(ASSET_DIR)/images/* $(ASSET_DIR)/models/*)
ASSET_HEADERS = $(addprefix $(ASSET_OUT)/,icon_assets.h font_assets.h model_assets.h)
ASSET_OUTPUTS = $(ASSET_HEADERS) $(ASSET_OUT)/manifest.json
TARGET_FLAGS = -march=i8086 -mcmodel=small -mrtd -std=gnu11 -ffreestanding \
	-fno-common -fno-toplevel-reorder -ffunction-sections

.PHONY: all assets toolchain clean run
all: $(OUT)/Sentinel.exe $(addprefix $(OUT)/,$(notdir $(wildcard resources/*.bin)))
assets: $(ASSET_OUTPUTS)

# One invocation owns all generated outputs, including quantized model previews.
$(ASSET_OUTPUTS) &: compile_assets.py $(ASSET_INPUTS)
	$(PYTHON) compile_assets.py --assets $(ASSET_DIR) --out $(ASSET_OUT)

$(OUT):
	mkdir -p $@

$(OUT)/sentinel.o: sentinel.c Makefile $(wildcard *.c *.h) $(ASSET_HEADERS) | $(OUT)
	$(TOOLCHAIN) gcc $(TARGET_FLAGS) $(CFLAGS) -I$(abspath $(ASSET_OUT)) -c $< -o $@

SECONDARY_SOURCES = software_cursor.c ray_trace.c polygon_edges.c legacy_typed.c text_interpreter.c render_geometry.c graphics_ui.c palette.c render_world.c scene_flow.c input_controller.c
GAMEPLAY_SOURCES = player_actions.c sentry_actions.c
GAMEPLAY_OBJECTS = $(addprefix $(OUT)/,$(GAMEPLAY_SOURCES:.c=.o))
SECONDARY_OBJECTS = $(addprefix $(OUT)/,$(SECONDARY_SOURCES:.c=.o))

$(SECONDARY_OBJECTS) $(GAMEPLAY_OBJECTS): $(OUT)/%.o: %.c runtime_support.c Makefile $(wildcard *.h) $(ASSET_HEADERS) | $(OUT)
	$(TOOLCHAIN) gcc $(TARGET_FLAGS) $(CFLAGS) -I$(abspath $(ASSET_OUT)) -c $< -o $@

$(OUT)/Sentinel.elf: $(OUT)/sentinel.o $(SECONDARY_OBJECTS) $(GAMEPLAY_OBJECTS) sentinel.ld
	$(TOOLCHAIN) ld --no-check-sections --emit-relocs -T sentinel.ld -Map $(OUT)/Sentinel.map -o $@ $(OUT)/sentinel.o $(SECONDARY_OBJECTS) $(GAMEPLAY_OBJECTS) $(shell $(TOOLCHAIN) gcc $(TARGET_FLAGS) -print-libgcc-file-name)

$(OUT)/Sentinel.exe: $(OUT)/Sentinel.elf finish_mz.py
	$(PYTHON) finish_mz.py $< $@ $(TOOLCHAIN)

$(OUT)/%.bin: resources/%.bin | $(OUT)
	cp $< $@

toolchain:
	$(PYTHON) tools/install_ia16.py

clean:
	rm -rf $(OUT)

DOSBOX ?= dosbox-x
run: all
	$(DOSBOX) -defaultconf -fastlaunch -nogui -nomenu -set "cpu cycles=6000" -c 'mount c "$(abspath $(OUT))"' -c "c:" -c "Sentinel.exe" -c "exit"
