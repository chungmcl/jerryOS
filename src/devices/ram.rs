use crate::devices::*;

pub enum RAMSetupError {
    GetPropertyRegWasNull(FDTError)
}

pub fn load_ram_specs(ram_node: FDTNode) -> Result<(*const u8, u64), RAMSetupError> { 
    // RAM specs are stored at the memory@... DTB node in the "reg" property as u64s. 
    // Unfortunately, the u64s are not guaranteed to be aligned. Therefore, we get a slice to
    // the two u64s we want as a slice of 16 u8s first, and then copy them out to the stack aligned 
    // via [i..i+8].try_into() before converting from BE to LE.
    // If we called get_node_property::<u64>, we'd get a
    // "unsafe precondition(s) violated: slice::from_raw_parts requires the pointer to be aligned..."
    // panic.

    let regs: &'static [u8] = match ram_node.get_property::<u8>(b"reg\0") {
        Ok(regs) => regs,
        Err(e) => { return Err(RAMSetupError::GetPropertyRegWasNull(e)); }
    };

    let ram_start: *const u8 = u64::from_be_bytes(regs[0..8].try_into().unwrap()) as *const u8;
    let ram_len: u64 = u64::from_be_bytes(regs[8..16].try_into().unwrap()) as u64;

    return Ok((ram_start, ram_len));
}
