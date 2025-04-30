BUILD_DIR="build"
DISK_PATH="${BUILD_DIR}/disk.img"

# Note: use "-machine virt,virtualization=on" to enable EL2
make \
&& \
qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a710 \
  -m 1G \
  -drive file="${DISK_PATH}",if=none,format=raw,id=vd -device virtio-blk-device,drive=vd -global virtio-mmio.force-legacy=false \
  -kernel build/jerryOS -S \
  -gdb tcp::4321 \
  -nographic
