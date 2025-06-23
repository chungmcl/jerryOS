pub mod ram;
pub mod virtio;
pub mod libfdt_lite;
pub use libfdt_lite::*;

pub use core::ffi::{c_void, c_int};
pub use core::{slice, ptr};
pub use num_enum::FromPrimitive;
use virtio::VirtIOError;

pub enum DeviceSetupError {
    LibFDTInitFailed(FDTError),
    FDTItrNewFailed(FDTError),
    RAMSetup(FDTError),
    VirtIOSetup(virtio::VirtIOError)
}
 
pub fn init_devices(kernel_dtb_start: *const u8) -> Result<(*const u8, *const u8, usize), DeviceSetupError> {
    if let Err(e) = libfdt_lite_init(kernel_dtb_start) {
        return Err(DeviceSetupError::LibFDTInitFailed(e));
    }

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

    // These are set at run-time in ram::load_ram_specs()
    let mut ram_start: *const u8 = ptr::null_mut();
    let mut ram_len: usize = 0;

    let mut fdt = match FDTItr::new() {
        Ok(fdt) => fdt,
        Err(e) => { return Err(DeviceSetupError::FDTItrNewFailed(e)); }
    };


    for device in &mut fdt {
        match device.get_name() {
            Ok(name) => {
                match name {
                    name if name.starts_with("memory@") => {
                        match ram::load_ram_specs(device) {
                            Ok((_ram_start, _ram_len)) => {
                                ram_start = _ram_start;
                                ram_len = _ram_len;
                            }
                            Err(e) => return Err(DeviceSetupError::RAMSetup(e))
                        }
                    },
                    name if name.starts_with("virtio_mmio") => {
                        match virtio::setup_virtio_device(device) {
                            Ok(_virtio_device) => {
                                // TODO(chungmcl): do... stuff? with the virtio_device
                            },
                            Err(VirtIOError::UnsupportedDeviceType) => { /* skip device; do nothing */ },
                            Err(e) => return Err(DeviceSetupError::VirtIOSetup(e))
                        }
                    },
                    _ => { }
                }
            },
            Err(_e) => {
                // TODO(chungmcl): Log that one of the devices' names was were unparsable,
                // but we don't need to return/stop parsing the DTB
            }
        }
    }
    return Ok((kernel_dtb_end, ram_start, ram_len)); 
}
