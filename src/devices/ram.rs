use crate::devices::*;

pub unsafe fn load_ram_specs(jerry_meta_data: &mut JerryMetaData, ram_node_offset: i32) -> bool { 
    unsafe {
        let mut reg_len: i32 = 0;
        let reg_ptr: *const u64 = fdt_getprop(
            jerry_meta_data.kernel_dtb_start as *const c_void, 
            ram_node_offset as c_int, 
            "reg\0".as_ptr(), 
            &mut reg_len as *mut i32
        ) as *const u64;
        if reg_ptr != ptr::null() {
            jerry_meta_data.ram_start = u64::from_be(reg_ptr.read_unaligned()) as *const u8;
            jerry_meta_data.ram_len = u64::from_be(reg_ptr.add(1).read_unaligned());
            true
        } else {
            false
        }
    }
}
