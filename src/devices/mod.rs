#[link(name = "libfdtLite", kind = "static")] unsafe extern "C" {}
#[allow(unsafe_op_in_unsafe_fn, non_snake_case, non_camel_case_types, non_upper_case_globals, improper_ctypes, dead_code)]
pub mod libfdtLite {
    include!("../CBindings/libfdtLite.rs");
}
pub mod ram;
pub use crate::types::JerryMetaData;
pub use core::ffi::{c_void, c_int};
pub use core::ptr;
use core::slice;
pub use libfdtLite::*;

pub fn init_devices(jerry_meta_data: &mut JerryMetaData) -> bool {
    unsafe {
        if fdt_check_header(jerry_meta_data.kernel_dtb_start as *const c_void) != 0 {
            panic!("Device setup failed!");
        }
        jerry_meta_data.kernel_dtb_end = 
            jerry_meta_data.kernel_dtb_start.add( 
                (
                    *(jerry_meta_data.kernel_dtb_start as *const fdt_header)
                )
                .totalsize as usize
            )
        ;
        
        let mut cur_node_offset: i32 = 0;
        let mut _depth: c_int = 0;
        loop {
            cur_node_offset = fdt_next_node(
                jerry_meta_data.kernel_dtb_start as *const c_void,
                cur_node_offset,
                &mut _depth
            );
            if cur_node_offset < 0 { 
                break; 
            }

            let mut cur_node_name_len: c_int = 0;
            let cur_node_name_ptr = fdt_get_name(
                jerry_meta_data.kernel_dtb_start as *const c_void, 
                cur_node_offset, 
                &mut cur_node_name_len
            );

            if cur_node_name_len >= 0 {
                let cur_node_name_bytes = slice::from_raw_parts(cur_node_name_ptr, cur_node_name_len as usize);
                let name = str::from_utf8_unchecked(cur_node_name_bytes);
    
                match name {
                    name if name.starts_with("memory@") => {
                        if !ram::load_ram_specs(jerry_meta_data, cur_node_offset) {
                            return false
                        }
                    },
                    name if name.starts_with("virtio_mmio") => {
    
                    },
                    _ => { }
                }
            } else {
                match cur_node_name_len {
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_NOTFOUND as i32)     => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_EXISTS as i32)       => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_NOSPACE as i32)      => {},        
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADOFFSET as i32)    => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADPATH as i32)      => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADPHANDLE as i32)   => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADSTATE as i32)     => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_TRUNCATED as i32)    => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADMAGIC as i32)     => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADVERSION as i32)   => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADSTRUCTURE as i32) => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADLAYOUT as i32)    => {}, 
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_INTERNAL as i32)     => {},        
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADNCELLS as i32)    => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADVALUE as i32)     => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADOVERLAY as i32)   => {}, 
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_NOPHANDLES as i32)   => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_BADFLAGS as i32)     => {},
                    cur_node_name_len if cur_node_name_len == -(FDT_ERR_ALIGNMENT as i32)    => {},      
                    _other => {
                        // Unknown error
                    }
                }
            }
        }
    }
    return true
}
