pub mod ttd;
pub use core::ffi::{c_void, c_int};
pub use core::{slice, ptr};
use ttd::*;

#[unsafe(link_section = ".kernel_root_tables")] #[unsafe(no_mangle)]
pub static mut KERNEL_ROOT_TABLE0: [TableDescriptorS1; 8] = [TableDescriptorS1::new(); 8];

#[unsafe(link_section = ".kernel_root_tables")] #[unsafe(no_mangle)]
pub static mut KERNEL_ROOT_TABLE1: [TableDescriptorS1; 8] = [TableDescriptorS1::new(); 8];

pub enum MemorySetupError {
    E
}
 
pub fn init_memory() -> Result<(), MemorySetupError> {
    let test: TableDescriptorS1 = TableDescriptorS1::new().with_valid_bit(true);
    unsafe {
        KERNEL_ROOT_TABLE0[0] = test;
    }
    Ok(())
}
