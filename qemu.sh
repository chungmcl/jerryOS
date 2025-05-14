# Uncomment this to print out exactly what this script is running
# set -x

BUILD_DIR="build"
DISK_PATH="${BUILD_DIR}/disk.img"
LLDB_PORT=$(sed -nE "s/.*gdb-remote localhost:([0-9]+)/\1/p" ./lldb-startup.txt)
MEMORY_GB=$(sed -nE "s/.*MEMORY_SIZE_GB[[:space:]]+([0-9]+)/\1/p" ./src/include/meta.h)

if [ -z "$LLDB_PORT" ]; then
  echo "Error: A valid port in \"gdb-remote localhost:port\" not found in ./lldb-startup.txt!"
  exit 1
fi

if [ -z "$MEMORY_GB" ]; then
  echo "Error: A valid value for MEMORY_SIZE_GB not found in ./src/include/meta.h!"
  exit 1
fi

# Note: use "-machine virt,virtualization=on" to enable EL2
# https://qemu-project.gitlab.io/qemu/system/arm/virt.html
make \
&& \
echo "Starting QEMU on localhost:$LLDB_PORT with $MEMORY_GB GB of RAM." \
&& \
qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a710 \
  -m ${MEMORY_GB}G \
  -drive file="${DISK_PATH}",if=none,format=raw,id=vd -device virtio-blk-device,drive=vd -global virtio-mmio.force-legacy=false \
  -kernel build/jerryOS -S \
  -gdb tcp::${LLDB_PORT} \
  -nographic
