use crate::devices::*;

pub fn load_ram_specs(ram_node: FDTNode) -> Result<(*const u8, usize), FDTError> {
    match ram_node.get_reg() {
        Ok((ram_start, ram_len)) => { return Ok((ram_start as *const u8, ram_len as usize)); },
        Err(e) => { return Err(e); }
    }
}
