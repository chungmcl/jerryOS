use core::{ptr, arch};
use arch::asm;

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

#[inline(always)]
pub fn write32(reg: &mut u32, val: u32) {
    unsafe {
        ptr::write_volatile(reg as *mut u32, val);
    }
}

#[inline(always)]
pub unsafe fn read64(reg: &u64) -> u64 {
    unsafe { 
        return ptr::read_volatile(reg as *const u64);
    }
}

#[inline(always)]
pub fn write64(reg: &mut u64, val: u64) {
    unsafe {
        ptr::write_volatile(reg as *mut u64, val);
    }
}

// Synchronization Barrier Type
#[derive(Copy, Clone)]
pub enum SBType {
    Sy,
    St,
    Ld,
}

#[inline(always)]
pub unsafe fn dsb(barrier_type: SBType) {
    unsafe {
        match barrier_type {
            SBType::Sy => asm!("dsb sy", options(nostack, preserves_flags)),
            SBType::St => asm!("dsb st", options(nostack, preserves_flags)),
            SBType::Ld => asm!("dsb ld", options(nostack, preserves_flags)),
        }
    }
}

#[inline(always)]
pub unsafe fn isb(barrier_type: SBType) {
    unsafe {
        match barrier_type {
            SBType::Sy => asm!("isb sy", options(nostack, preserves_flags)),
            SBType::St => asm!("isb st", options(nostack, preserves_flags)),
            SBType::Ld => asm!("isb ld", options(nostack, preserves_flags)),
        }
    }
}

#[macro_export]
macro_rules! break_dummy {() => { { let _: u32 = 2 + 2; } }; }