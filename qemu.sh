# Uncomment this to print out exactly what this script is running
# set -x

if ! ./build.sh; then
  echo "Build failed!"
  exit 1
fi
BUILD_DIR="target/aarch64-unknown-jerryOS-elf"
echo "---------------------------------------"

MEMORY_GB=1
DISK_GB=1
DISK_DIR=.disks
DISK_PATH=${DISK_DIR}/disk_${DISK_GB}G.img
mkdir -p ${DISK_DIR}
if [ ! -f "${DISK_PATH}" ]; then 
  echo ""
  echo "disk_${DISK_GB}.img not already present. Creating with qemu-img create:"
  qemu-img create -f raw "${DISK_PATH}" "${DISK_GB}G"
  echo ""
fi

LLDB_PORT=$(sed -nE "s/.*gdb-remote localhost:([0-9]+)/\1/p" ./lldb-startup.txt)
if [ -z "$LLDB_PORT" ]; then
  echo "Error: A valid port in \"gdb-remote localhost:port\" not found in ./lldb-startup.txt!"
  exit 1
fi

# Note: use "-machine virt,virtualization=on" to enable EL2
# https://qemu-project.gitlab.io/qemu/system/arm/virt.html
echo "Starting QEMU on localhost:$LLDB_PORT with $MEMORY_GB GB of RAM & a $DISK_GB GB disk." \
&& \
qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a710 \
  -m ${MEMORY_GB}G \
  -drive file="${DISK_PATH}",if=none,format=raw,id=vd -device virtio-blk-device,drive=vd -global virtio-mmio.force-legacy=false \
  -kernel ${BUILD_DIR}/debug/jerryOS -S \
  -gdb tcp::${LLDB_PORT} \
  -nographic
