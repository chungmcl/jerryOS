RUSTC_BOOTSTRAP=1
DISK_DIR=.disks
DISK_PATH=${DISK_DIR}/disk.img

mkdir -p ${DISK_DIR}
if [ ! -f "${DISK_PATH}" ]; then echo "\ndisk.img not already present. Creating with qemu-img create:"; qemu-img create -f raw "${DISK_PATH}" 1G; echo "\n"; fi

rustup override set nightly
rustup component add rust-src
# cargo +nightly build -Z build-std=core --target=aarch64-unknown-jerryOS-elf.json --verbose
cargo +nightly build -Z build-std=core --target=aarch64-unknown-jerryOS-elf.json
