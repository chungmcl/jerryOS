pub mod libfdt_lite;
pub mod pl011_uart;
pub mod memory;
pub mod virtio;

pub use core::ffi::{c_void, c_int};
pub use core::{slice, ptr};
pub use num_enum::FromPrimitive;
pub use super::*;
pub use libfdt_lite::*;
pub use memory::{ptm::{map_mmio_range, PTMError}, MemoryError};
pub use virtio::VirtIOError;
pub use pl011_uart::PL011Error;
use memory::{init_memory};
use crate::{println, JerryMetaData};

pub enum DeviceInitError {
    LibFDTInitFailed(FDTError),
    FDTItrNewFailed(FDTError),
    MemoryDeviceNotFound,
    SearchForMemoryDeviceFailed(FDTError),
    MemoryInitFailed(MemoryError),
    VirtIOSetup(VirtIOError),
    PL011Setup(PL011Error)
}
 
pub fn init_devices(kernel_meta_data: JerryMetaData) -> Result<(), DeviceInitError> {
    if let Err(e) = libfdt_lite_init(kernel_meta_data.kernel_dtb_start) {
        return Err(DeviceInitError::LibFDTInitFailed(e));
    }
    let kernel_dtb_start: *const u8 = kernel_meta_data.kernel_dtb_start;

    /* 
     * Frustratingly, kernel_dtb_start is 0x0 on QEMU sometimes 🫠 So doing:
     * 1 kernel_dtb_end = 
     * 2    kernel_dtb_start.add(
     * 3        (
     * 4            *(kernel_dtb_start as *const fdt_header)
     * 5        )
     * 6        .totalsize as usize
     * 7    );
     * unfortunately causes a null pointer dereference panic on line 4.
     * Thankfully the first byte of the header is just the magic number, and 
     * the DTB length is a u32 located at +0x4 after kernel_dtb_start,
     * so the below code is a hacky solution to this problem.
    */
    let dtb_total_size: u32 = unsafe { *(kernel_dtb_start.add(4) as *const u32) };
    let kernel_dtb_end: *const u8 = unsafe { kernel_dtb_start.add(dtb_total_size as usize) as *const u8 };

    for device in match FDTItr::new() {
        Ok(fdt) => fdt,
        Err(e) => { return Err(DeviceInitError::FDTItrNewFailed(e)); }
    } {
        // Memory must be enabled before anything else since we will need all other devices
        // to be running in virtual address space
        if match device.get_name() {
            Ok(device_name) => {device_name},
            Err(e) => { return Err(DeviceInitError::SearchForMemoryDeviceFailed(e)); }
        }.starts_with("memory@") {
            match init_memory(
                device, 
                kernel_meta_data.kernel_text_end,
                kernel_meta_data.kernel_dtb_start,
                kernel_dtb_end
            ) {
                Ok(_) => {
                    match call_device_inits() {
                        Ok(_) => {
                            return Ok(());
                        },
                        Err(e) => {
                            return Err(e);
                        }
                    }
                },
                Err(e) => {
                    return Err(DeviceInitError::MemoryInitFailed(e));
                }
            }
        }
    }
    return Err(DeviceInitError::MemoryDeviceNotFound);
}

pub fn call_device_inits() -> Result<(), DeviceInitError> {
    let mut fdt = match FDTItr::new() {
        Ok(fdt) => fdt,
        Err(e) => { return Err(DeviceInitError::FDTItrNewFailed(e)); }
    };
    for device in &mut fdt {
        match device.get_name() {
            Ok(name) => {
                match name {
                    name if name.starts_with("virtio_mmio") => {
                        match virtio::init_virtio_device(device) {
                            Ok(_virtio_device) => {
                                // TODO(chungmcl): do... stuff? with the virtio_device
                            },
                            Err(VirtIOError::UnsupportedDeviceType) => { /* skip device; do nothing */ },
                            Err(e) => return Err(DeviceInitError::VirtIOSetup(e))
                        }
                    },
                    name if name.starts_with("pl011") => {
                        match pl011_uart::init_pl011_uart(device) {
                            Ok(_pl011_mmio_addy) => {

                            },
                            Err(e) => return Err(DeviceInitError::PL011Setup(e))
                        }
                    },
                    _ => { }
                }
            },
            Err(e) => {
                println!("call_device_init() failed on a device with \"FDTError::{:?}]\"!", e);
            }
        }
    }
    return Ok(()); 
}
