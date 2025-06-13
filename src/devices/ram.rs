use crate::devices::*;

pub fn load_ram_specs(ram_node: FDTNode) -> Result<(*const u8, u64), FDTError> { 
    // RAM specs are stored at the memory@... DTB node in the "reg" property as u64s. 
    // Unfortunately, the u64s are not guaranteed to be aligned. Therefore, we get a slice to
    // the two u64s we want as a slice of 16 u8s first, and then copy them out to the stack aligned 
    // via [i..i+8].try_into() before converting from BE to LE.
    // If we called get_node_property::<u64>, we'd get a
    // "unsafe precondition(s) violated: slice::from_raw_parts requires the pointer to be aligned..."
    // panic.

    match ram_node.get_reg() {
        Ok((ram_start, ram_len)) => { return Ok((ram_start as *const u8, ram_len as u64)); },
        Err(e) => { return Err(e); }
    }
}
