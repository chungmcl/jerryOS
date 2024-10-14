BUILD_DIR="build"
DISK_PATH="${BUILD_DIR}/disk.img"

make \
&& \
qemu-system-aarch64 -machine virt -cpu max -m 1G -kernel build/jerryOS.bin -S -gdb tcp::4321 -nographic -drive file="${DISK_PATH}",if=none,id=hd0,format=raw -device virtio-blk,drive=hd0
