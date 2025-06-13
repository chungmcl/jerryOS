pub mod ttd;
pub use core::ffi::{c_void, c_int};
pub use core::{slice, ptr};

pub enum MemorySetupError {
    
}

 
pub fn init_memory() -> Result<(), MemorySetupError> {
    Ok(())
}
