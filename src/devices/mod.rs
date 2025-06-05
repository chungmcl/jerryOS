use crate::types::JerryMetaData;
#[link(name = "libfdtLite", kind = "static")] unsafe extern "C" {}
#[allow(unsafe_op_in_unsafe_fn, non_snake_case, non_camel_case_types, non_upper_case_globals, improper_ctypes, dead_code)]
mod libfdtLite {
    include!("../CBindings/libfdtLite.rs");
}
use core::ffi::{c_void, c_int};
use core::ptr;
use core::slice;
use libfdtLite::*;

pub fn init_devices(jerry_meta_data: &mut JerryMetaData) {
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
        
        let fdt: *const c_void = jerry_meta_data.kernel_dtb_start as *const c_void;
        let mut curr_node_offset: i32 = 0;
        let mut _depth: c_int = 0;
        loop {
            curr_node_offset = fdt_next_node(
                fdt,
                curr_node_offset,
                &mut _depth
            );

            if curr_node_offset < 0 { break; }
            let cur_node_name = fdt_get_name(fdt, curr_node_offset, ptr::null_mut());
            let mut len = 0;
            while *cur_node_name.add(len) != 0 {
                len += 1;
            }
            let bytes = slice::from_raw_parts(cur_node_name, len);
            let name = str::from_utf8_unchecked(bytes);
        }
    }
}
