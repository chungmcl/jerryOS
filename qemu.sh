# Uncomment this to print out exactly what this script is running
# set -x

if ! ./build.sh; then
  echo "Build failed!"
  exit 1
fi
BUILD_DIR="target/aarch64-unknown-jerryOS-elf"
echo "--------------------------------------------------------------------"

MEMORY_N=512
MEMORY_UNIT=M
DISK_N=1
DISK_UNIT=G
DISK_DIR=.disks
DISK_PATH=${DISK_DIR}/disk_${DISK_N}${DISK_UNIT}.img
mkdir -p ${DISK_DIR}
if [ ! -f "${DISK_PATH}" ]; then 
  echo ""
  echo "disk_${DISK_N}${DISK_UNIT}.img not already present. Creating with qemu-img create:"
  qemu-img create -f raw "${DISK_PATH}" "${DISK_GB}${DISK_UNIT}"
  echo ""
fi

LLDB_PORT=$(sed -nE "s/.*gdb-remote localhost:([0-9]+)/\1/p" ./lldb-startup.txt)
if [ -z "$LLDB_PORT" ]; then
  echo "Error: A valid port in \"gdb-remote localhost:port\" not found in ./lldb-startup.txt!"
  exit 1
fi

# Note: use "-machine virt,virtualization=on" to enable EL2
# https://qemu-project.gitlab.io/qemu/system/arm/virt.html
echo "Starting QEMU on localhost:${LLDB_PORT} with ${MEMORY_N} ${MEMORY_UNIT}B of RAM & a ${DISK_N} ${DISK_UNIT}B disk." \
&& \
echo "--------------------------------------------------------------------" \
&& \
qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a710 \
  -m ${MEMORY_N}${MEMORY_UNIT} \
  -drive file="${DISK_PATH}",if=none,format=raw,id=vd -device virtio-blk-device,drive=vd -global virtio-mmio.force-legacy=false \
  -serial mon:stdio \
  -kernel ${BUILD_DIR}/debug/jerryOS -S \
  -gdb tcp::${LLDB_PORT} \
  -nographic
