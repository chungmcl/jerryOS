pub mod ttd;
pub mod ppm;
pub mod ptm;
pub use crate::types::*;
pub use core::ptr;
pub use ttd::*;
use ppm::*;
use ptm::*;

pub enum MemorySetupError {
    PPMInitFailed(PPMError)
}
 
pub fn init_memory(
    static_kernel_mem_end: *const u8, 
    ram_start: *const u8, 
    ram_len: u64
) -> Result<(), MemorySetupError> {
    match init_ppm(static_kernel_mem_end, ram_start, ram_len) {
        Ok(_) => { },
        Err(e) => {
            return Err(MemorySetupError::PPMInitFailed(e));
        }
    }
    
    let test: TableDescriptorS1 = TableDescriptorS1::new().with_valid_bit(true);
    
    unsafe {
        KERNEL_ROOT_TABLE0[0] = test;
    }
    Ok(())
}
