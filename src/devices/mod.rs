#[link(name = "libfdtLite", kind = "static")] unsafe extern "C" {}
#[allow(unsafe_op_in_unsafe_fn, non_snake_case, non_camel_case_types, non_upper_case_globals, improper_ctypes, dead_code)]
pub mod libfdtLite {
    include!("../CBindings/libfdtLite.rs");
}
pub mod ram;
pub mod virtio;
pub use core::ffi::{c_void, c_int};
pub use core::{mem, slice, ptr};
pub use libfdtLite::*;
pub use num_enum::FromPrimitive;

pub enum DeviceSetupError {
    RAMSetup,
    VirtIOSetup
}
 
static mut KERNEL_DTB_START: *const u8 = ptr::null_mut();
 
pub fn init_devices(kernel_dtb_start: *const u8) -> Result<(*const u8, *const u8, u64), DeviceSetupError> {
    unsafe {
        KERNEL_DTB_START = kernel_dtb_start;
        let mut kernel_dtb_end: *const u8 = ptr::null_mut();
        let mut ram_start: *const u8 = ptr::null_mut();
        let mut ram_len: u64 = 0;

        let check_header_ret: i32 = fdt_check_header(KERNEL_DTB_START as *const c_void);
        if check_header_ret != 0 {
            let _fdt_error: FDTError = FDTError::from(check_header_ret);
            panic!("Device setup failed!");
        }

        /* Frustratingly, KERNEL_DTB_START is 0x0 on QEMU sometimes 🫠 So doing:
            // 1 kernel_dtb_end = 
            // 2     KERNEL_DTB_START.add( 
            // 3         (
            // 4              *(KERNEL_DTB_START as *const fdt_header)
            // 5         )
            // 6         .totalsize as usize
            // 7     );
        unfortunately causes a null pointer dereference panic on line 4.
        Thankfully the first byte of the header is just the magic number, and 
        the DTB length is a u32 located at +0x4 after KERNEL_DTB_START,
        so the below code is a hacky solution to this problem.
        */
        let dtb_total_size: u32 = *(KERNEL_DTB_START.add(4) as *const u32);
        kernel_dtb_end = KERNEL_DTB_START.add(dtb_total_size as usize);
        
        let mut cur_node_offset: i32 = 0;
        let mut _depth: c_int = 0;
        loop {
            cur_node_offset = fdt_next_node(
                KERNEL_DTB_START as *const c_void,
                cur_node_offset,
                &mut _depth
            );
            if cur_node_offset < 0 { 
                break; 
            }

            let mut cur_node_name_len: c_int = 0;
            let cur_node_name_ptr = fdt_get_name(
                KERNEL_DTB_START as *const c_void, 
                cur_node_offset, 
                &mut cur_node_name_len
            );

            if cur_node_name_len >= 0 {
                let cur_node_name_bytes = slice::from_raw_parts(cur_node_name_ptr, cur_node_name_len as usize);
                let name = str::from_utf8_unchecked(cur_node_name_bytes);
    
                match name {
                    name if name.starts_with("memory@") => {
                        match ram::load_ram_specs(cur_node_offset) {
                            Ok((_ram_start, _ram_len)) => {
                                ram_start = _ram_start;
                                ram_len = _ram_len;
                            }
                            Err(_e) => { 
                                return Err(DeviceSetupError::RAMSetup);
                            }
                        }
                    },
                    name if name.starts_with("virtio_mmio") => {
                        
                    },
                    _ => { }
                }
            } else {
                match FDTError::from(cur_node_name_len) {
                    FDTError::NotFound     => {},
                    FDTError::Exists       => {},
                    FDTError::NoSpace      => {},        
                    FDTError::BadOffset    => {},
                    FDTError::BadPath      => {},
                    FDTError::BadPhandle   => {},
                    FDTError::BadState     => {},
                    FDTError::Truncated    => {},
                    FDTError::BadMagic     => {},
                    FDTError::BadVersion   => {},
                    FDTError::BadStructure => {},
                    FDTError::BadLayout    => {}, 
                    FDTError::Internal     => {},        
                    FDTError::BadNCells    => {},
                    FDTError::BadValue     => {},
                    FDTError::BadOverlay   => {}, 
                    FDTError::NoPhandles   => {},
                    FDTError::BadFlags     => {},
                    FDTError::Alignment    => {},      
                    _other => {
                        // Unknown error
                    }
                }
            }
        }
        Ok((kernel_dtb_end, ram_start, ram_len))
    }
}

#[derive(FromPrimitive)]
#[repr(i32)]
pub enum FDTError {
    #[num_enum(default)] Unknown,
    NotFound     = -(FDT_ERR_NOTFOUND as i32),    
    Exists       = -(FDT_ERR_EXISTS as i32),      
    NoSpace      = -(FDT_ERR_NOSPACE as i32),     
    BadOffset    = -(FDT_ERR_BADOFFSET as i32),   
    BadPath      = -(FDT_ERR_BADPATH as i32),     
    BadPhandle   = -(FDT_ERR_BADPHANDLE as i32),  
    BadState     = -(FDT_ERR_BADSTATE as i32),    
    Truncated    = -(FDT_ERR_TRUNCATED as i32),   
    BadMagic     = -(FDT_ERR_BADMAGIC as i32),    
    BadVersion   = -(FDT_ERR_BADVERSION as i32),  
    BadStructure = -(FDT_ERR_BADSTRUCTURE as i32),
    BadLayout    = -(FDT_ERR_BADLAYOUT as i32),   
    Internal     = -(FDT_ERR_INTERNAL as i32),    
    BadNCells    = -(FDT_ERR_BADNCELLS as i32),   
    BadValue     = -(FDT_ERR_BADVALUE as i32),    
    BadOverlay   = -(FDT_ERR_BADOVERLAY as i32),  
    NoPhandles   = -(FDT_ERR_NOPHANDLES as i32),  
    BadFlags     = -(FDT_ERR_BADFLAGS as i32),    
    Alignment    = -(FDT_ERR_ALIGNMENT as i32)
}

unsafe fn get_node_property<T>(node_offset: i32, property_name: &[u8]) -> Result<&'static [T], FDTError>  {
    unsafe {
        let mut lenp: i32 = 0;
        let reg_ptr: *const u8 = fdt_getprop(
            KERNEL_DTB_START as *const c_void, 
            node_offset as c_int, 
            property_name.as_ptr(), 
            &mut lenp as *mut i32
        ) as *const u8;
        if lenp < 0 {
            Err(FDTError::from(lenp))
        } else {
            let element_count = (lenp as usize) / mem::size_of::<T>();
            Ok(slice::from_raw_parts(reg_ptr as *const T, element_count))
        }
    }
}
