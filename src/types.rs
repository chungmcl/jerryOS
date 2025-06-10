use core::ptr;

pub struct JerryMetaData {
    pub kernel_dtb_start    : *const u8,
    pub kernel_dtb_end      : *const u8,
    pub kernel_init_sp      : *const u8,
    pub kernel_bin_start    : *const u8,
    pub kernel_rodata_start : *const u8,
    pub kernel_rodata_end   : *const u8,
    pub kernel_text_start   : *const u8,
    pub kernel_text_end     : *const u8,
    pub kernel_bss_start    : *const u8,
    pub kernel_bss_end      : *const u8,
    pub ram_start           : *const u8,
    pub ram_len             : u64
}

#[inline(always)]
pub unsafe fn read32(reg: &u32) -> u32 {
    unsafe { 
        return ptr::read_volatile(reg as *const u32);
    }
}

#[macro_export]
macro_rules! break_dummy {() => { { let _: u32 = 2 + 2; } }; }