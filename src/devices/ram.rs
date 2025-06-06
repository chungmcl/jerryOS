use crate::devices::*;

pub enum RAMSetupError {
    GetPropertyRegWasNull
}

pub unsafe fn load_ram_specs(ram_node_offset: i32) -> Result<(*const u8, u64), FDTError> { 
    let regs: &'static [u8] = get_node_property::<u8>(ram_node_offset, b"reg\0")?;
    let ram_start_bytes: [u8; 4] = regs[0..4].try_into().unwrap();
    let ram_len_bytes: [u8; 4] = regs[4..8].try_into().unwrap();

    let ram_start = u32::from_be_bytes(ram_start_bytes) as *const u8;
    let ram_len = u32::from_be_bytes(ram_len_bytes) as u64;

    return Ok((ram_start, ram_len));

    //     let mut reg_len: i32 = 0;
    //     let reg_ptr: *const u64 = fdt_getprop(
    //         jerry_meta_data.kernel_dtb_start as *const c_void, 
    //         ram_node_offset as c_int, 
    //         "reg\0".as_ptr(), 
    //         &mut reg_len as *mut i32
    //     ) as *const u64;
    //     if reg_ptr != ptr::null() {
    //         jerry_meta_data.ram_start = u64::from_be(reg_ptr.read_unaligned()) as *const u8;
    //         jerry_meta_data.ram_len = u64::from_be(reg_ptr.add(1).read_unaligned());
    //         Ok((
    //             u64::from_be(reg_ptr.read_unaligned()) as *const u8, 
    //             u64::from_be(reg_ptr.add(1).read_unaligned())
    //         ))
    //     } else {
    //         Err(RAMSetupError::GetPropertyRegWasNull)
    //     }
    // }
}
