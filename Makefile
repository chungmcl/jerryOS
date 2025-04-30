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
# Note that -T only works with ELF, not with MachO -- hence the need for --target=aarch64-unknown-$(NAME)-elf
	clang -g -o $(BUILD_DIR)/$(NAME) \
		-I$(SRC_DIR)/include \
		-I$(SRC_DIR)/jerryLibc/include \
		-I$(SRC_DIR)/memory/include \
		-I$(SRC_DIR)/devices/libfdtLite/include \
		-I$(SRC_DIR)/devices/include \
		$(SRC_DIR)/start.asm \
		$(SRC_DIR)/main.c \
		$(SRC_DIR)/jerryLibc/string.c \
		$(SRC_DIR)/devices/libfdtLite/fdt.c $(SRC_DIR)/devices/libfdtLite/fdt_ro.c \
		$(SRC_DIR)/devices/devicesSetup.c $(SRC_DIR)/devices/virtio.c \
		--target=aarch64-unknown-$(NAME)-elf \
		-nostdlib -nostdinc -fno-builtin -static -v

	@echo "--------------------------------------------------"

clean:
	rm -rf $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)
