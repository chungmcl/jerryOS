NAME=jerryOS
SRC_DIR=src
BUILD_DIR=build
DISK_PATH=$(BUILD_DIR)/disk.img

all:
	export ARCHFLAGS="-arch arm64"
	mkdir -p $(BUILD_DIR)

	@if [ ! -f "$(DISK_PATH)" ]; then echo "\ndisk.img not already present. Creating with qemu-img create:"; qemu-img create -f raw "$(DISK_PATH)" 1G; echo "\n"; fi

	@echo "--------------------------------------------------"
# https://lld.llvm.org
# https://lld.llvm.org/ELF/linker_script.html
# -T <script>             Specify <script> as linker script
# Note that -T only works with ELF, not with MachO -- hence the need for --target=aarch64-unknown-linux-gnu
	clang -g -o $(BUILD_DIR)/$(NAME) \
		-I$(SRC_DIR)/include \
		-I$(SRC_DIR)/libfdtLite/include \
		$(SRC_DIR)/start.asm \
		$(SRC_DIR)/main.c \
		$(SRC_DIR)/libfdtLite/fdt.c \
		-T $(SRC_DIR)/link.lds \
		--target=aarch64-unknown-linux-gnu -nostdlib -static -v
	
	@echo "--------------------------------------------------"
# for some reason --only-section doesn't work but --dump-section does...? 
# but I still have to specify an output file even with --dump-section, so using a throwaway file named "sections.tmp"
	llvm-objcopy --dump-section ".text=$(BUILD_DIR)/$(NAME).bin" $(BUILD_DIR)/$(NAME) $(BUILD_DIR)/sections.tmp

	rm $(BUILD_DIR)/sections.tmp

	@echo "--------------------------------------------------"

clean:
	rm -rf $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)
