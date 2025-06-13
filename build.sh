RUSTC_BOOTSTRAP=1

rustup override set nightly
rustup component add rust-src
cargo +nightly build -Z build-std=core --target=aarch64-unknown-jerryOS-elf.json # --verbose
