#[link(name = "libfdtLite", kind = "static")] unsafe extern "C" {}
#[allow(unsafe_op_in_unsafe_fn, non_snake_case, non_camel_case_types, non_upper_case_globals, improper_ctypes, dead_code)]
mod libfdtLite {
    include!("../CBindings/libfdtLite.rs");
}
use libfdtLite::*;

use crate::devices::*;
static CELL_BYTES: usize = size_of::<u32>() as usize;
// TODO(chungmcl): convert these to OneCells or something similar.
static mut ADDRESS_CELLS: usize = 0;
static mut SIZE_CELLS: usize = 0;
static mut KERNEL_DTB_START: *const u8 = ptr::null_mut();
static mut INITIALIZED: bool = false;

#[derive(FromPrimitive)]
#[repr(i32)]
pub enum FDTError {
    #[num_enum(default)] Unknown,
    NotFound             = -(FDT_ERR_NOTFOUND as i32),    
    Exists               = -(FDT_ERR_EXISTS as i32),      
    NoSpace              = -(FDT_ERR_NOSPACE as i32),     
    BadOffset            = -(FDT_ERR_BADOFFSET as i32),   
    BadPath              = -(FDT_ERR_BADPATH as i32),     
    BadPhandle           = -(FDT_ERR_BADPHANDLE as i32),  
    BadState             = -(FDT_ERR_BADSTATE as i32),    
    Truncated            = -(FDT_ERR_TRUNCATED as i32),   
    BadMagic             = -(FDT_ERR_BADMAGIC as i32),    
    BadVersion           = -(FDT_ERR_BADVERSION as i32),  
    BadStructure         = -(FDT_ERR_BADSTRUCTURE as i32),
    BadLayout            = -(FDT_ERR_BADLAYOUT as i32),   
    Internal             = -(FDT_ERR_INTERNAL as i32),    
    BadNCells            = -(FDT_ERR_BADNCELLS as i32),   
    BadValue             = -(FDT_ERR_BADVALUE as i32),    
    BadOverlay           = -(FDT_ERR_BADOVERLAY as i32),  
    NoPhandles           = -(FDT_ERR_NOPHANDLES as i32),  
    BadFlags             = -(FDT_ERR_BADFLAGS as i32),    
    Alignment            = -(FDT_ERR_ALIGNMENT as i32),
    LibfdtNotInitialized = -(FDT_ERR_ALIGNMENT as i32) - 1,
    UnexpectedRegFormat  = -(FDT_ERR_ALIGNMENT as i32) - 2
}

pub fn libfdt_lite_init(kernel_dtb_start: *const u8) -> Result<(), FDTError> {
    unsafe {
        KERNEL_DTB_START = kernel_dtb_start;
        let check_header_ret: i32 = fdt_check_header(KERNEL_DTB_START as *const c_void);
        if check_header_ret != 0 {
            let fdt_error: FDTError = FDTError::from(check_header_ret);
            INITIALIZED = false;
            return Err(fdt_error);
        }

        let mut lenp: i32 = 0;
        let size_cells_ptr: *const u32 = fdt_getprop(
            KERNEL_DTB_START as *const c_void, 
            0 as c_int, 
            b"#size-cells\0".as_ptr(), &mut lenp as *mut i32
        ) as *const u32;
        if lenp < 0 { return Err(FDTError::from(lenp)); }
        SIZE_CELLS = u32::from_be(*size_cells_ptr) as usize;

        let address_cells_ptr: *const u32 = fdt_getprop(
            KERNEL_DTB_START as *const c_void, 
            0 as c_int, 
            b"#address-cells\0".as_ptr(), &mut lenp as *mut i32
        ) as *const u32;
        if lenp < 0 { return Err(FDTError::from(lenp)); }
        ADDRESS_CELLS = u32::from_be(*address_cells_ptr) as usize;

        INITIALIZED = true;
        return Ok(())
    }
}

#[derive(Clone)]
pub struct FDTNode {
    offset: i32,
    depth: i32
} impl FDTNode {
    pub fn get_name(&self) -> Result<&'static str, FDTError> {
        unsafe {
            let mut name_len: c_int = 0;
            let name_ptr: *const u8 = fdt_get_name(
                KERNEL_DTB_START as *const c_void, 
                self.offset, 
                &mut name_len
            );
            if name_len >= 0 {
                let name_bytes: &[u8] = slice::from_raw_parts(
                    name_ptr, 
                    name_len as usize
                );
                return Ok(str::from_utf8_unchecked(name_bytes));
            } else {
                return Err(FDTError::from(name_len));
            }
        }
    }

    pub fn get_property(&self, property_name: &[u8]) -> Result<&'static [u32], FDTError>  {
        unsafe {
            let mut lenp: i32 = 0;
            let reg_ptr: *const u8 = fdt_getprop(
                KERNEL_DTB_START as *const c_void, 
                self.offset as c_int, 
                property_name.as_ptr(), 
                &mut lenp as *mut i32
            ) as *const u8;
            if lenp < 0 {
                Err(FDTError::from(lenp))
            } else {
                Ok(slice::from_raw_parts(reg_ptr as *const u32, lenp as usize))
            }
        }
    }

    pub fn get_reg(&self) -> Result<(u64, u64), FDTError>  {
        unsafe {
            let mut lenp: i32 = 0;
            let reg_ptr: *const u8 = fdt_getprop(
                KERNEL_DTB_START as *const c_void, 
                self.offset as c_int, 
                b"reg\0".as_ptr(), 
                &mut lenp as *mut i32
            ) as *const u8;

            let address_bytes: usize = ADDRESS_CELLS * CELL_BYTES;
            let size_bytes: usize = SIZE_CELLS * CELL_BYTES;

            if lenp != (address_bytes + size_bytes) as i32 {
                if lenp < 0 && lenp > FDTError::UnexpectedRegFormat as i32 {
                    return Err(FDTError::from(lenp));
                } else {
                    return Err(FDTError::UnexpectedRegFormat);
                }
            }

            let address_slice: &[u8] = slice::from_raw_parts(
                reg_ptr, 
                address_bytes
            );
            let size_slice: &[u8] = slice::from_raw_parts(
                reg_ptr.add(address_bytes), 
                size_bytes
            );
            const U32_BYTES: usize = size_of::<u32>();
            const U64_BYTES: usize = size_of::<u64>();
            let address: u64 = match address_bytes {
                U32_BYTES => { 
                    u32::from_be_bytes(
                        address_slice[0..address_bytes]
                        .try_into().unwrap()
                    ) as u64 
                },
                U64_BYTES => { 
                    u64::from_be_bytes(
                        address_slice[0..address_bytes]
                        .try_into().unwrap()
                    )
                },
                _ => panic!("Invalid get_regs address bytes!")
            };
            let size: u64 = match size_bytes {
                U32_BYTES => { 
                    u32::from_be_bytes(
                        size_slice[0..size_bytes]
                        .try_into().unwrap()
                    ) as u64 
                },
                U64_BYTES => { 
                    u64::from_be_bytes(
                        size_slice[0..size_bytes]
                        .try_into().unwrap()
                    )
                },
                _ => panic!("Invalid get_regs size bytes!")
            };
            return Ok((address, size));
        }
    }
}

pub struct FDTItr {
    cur_node: FDTNode
} impl FDTItr {
    pub fn new() -> Result<Self, FDTError> {
        let initialized: bool = unsafe { INITIALIZED };
        if !initialized {
            return Err(FDTError::LibfdtNotInitialized);
        }

        return Ok(Self {
            cur_node: FDTNode { offset: 0, depth: 0 }
        });
    }
} impl Iterator for FDTItr {
    type Item = FDTNode;

    fn next(&mut self) -> Option<Self::Item> {
        unsafe {
            self.cur_node.offset = fdt_next_node(
                KERNEL_DTB_START as *const c_void,
                self.cur_node.offset,
                &mut self.cur_node.depth
            );
        }
        if self.cur_node.offset < 0 {
            None
        } else {
            Some(self.cur_node.clone())
        }
    }
}
