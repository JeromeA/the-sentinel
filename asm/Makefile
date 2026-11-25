NASM ?= nasm
PYTHON ?= python3
DOSBOX ?= dosbox-x
OUT ?= build
.DEFAULT_GOAL := all

.PHONY: all verify run clean
all: $(OUT)/Sentinel.exe $(addprefix $(OUT)/,$(notdir $(wildcard resources/*.bin)))

$(OUT):
	mkdir -p $@

$(OUT)/Sentinel.unpacked.exe: sentinel.asm | $(OUT)
	$(NASM) -f bin $< -o $@

$(OUT)/exepack-support.bin: sentinel.asm | $(OUT)
	$(NASM) -DEXEPACK_SUPPORT=1 -f bin $< -o $@

$(OUT)/Sentinel.exe: $(OUT)/Sentinel.unpacked.exe $(OUT)/exepack-support.bin exepack.py
	$(PYTHON) exepack.py $< $@ --support $(OUT)/exepack-support.bin

$(OUT)/%.bin: resources/%.bin | $(OUT)
	cp $< $@

verify: all
	printf '%s  %s\n' 0889629c699dc2bf070f5061884f611c24dcde992596aa724dd8ff581780d987 $(OUT)/Sentinel.exe | sha256sum -c -

run: all
	$(DOSBOX) -defaultconf -fastlaunch -nogui -nomenu -set "cpu cycles=6000" -c 'mount c "$(abspath $(OUT))"' -c "c:" -c "Sentinel.exe" -c "exit"

clean:
	rm -rf $(OUT)
