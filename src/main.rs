#![no_std]
#![no_main]
#[link(name = "entry")] unsafe extern "C" {}
use core::panic::PanicInfo;
use core::arch::asm;
mod types;
mod devices;
use crate::types::*;


#[unsafe(no_mangle)]
pub extern "C" fn main() -> ! {
    unsafe {
        let mut kernel_dtb          : u64;
        let mut kernel_init_sp      : u64;
        let mut kernel_bin_start    : u64;
        let mut kernel_rodata_start : u64;
        let mut kernel_rodata_end   : u64;
        let mut kernel_text_start   : u64;
        let mut kernel_text_end     : u64;
        let mut kernel_bss_start    : u64;
        let mut kernel_bss_end      : u64;
        asm!(
            "mov {0}, x1", "mov {1}, x2",
            "mov {2}, x3", "mov {3}, x4",
            "mov {4}, x5", "mov {5}, x6",
            "mov {6}, x7", "mov {7}, x8",
            "mov {8}, x9",
            out(reg) kernel_dtb,
            out(reg) kernel_init_sp,
            out(reg) kernel_bin_start,
            out(reg) kernel_rodata_start,
            out(reg) kernel_rodata_end,
            out(reg) kernel_text_start,
            out(reg) kernel_text_end,
            out(reg) kernel_bss_start,
            out(reg) kernel_bss_end,
            options(nostack, readonly)
        );

        let mut jerry_meta_data = JerryMetaData {
            kernel_dtb_start    : kernel_dtb          as *const u8,
            kernel_dtb_end      : 0                   as *const u8,
            kernel_init_sp      : kernel_init_sp      as *const u8,
            kernel_bin_start    : kernel_bin_start    as *const u8,
            kernel_rodata_start : kernel_rodata_start as *const u8,
            kernel_rodata_end   : kernel_rodata_end   as *const u8,
            kernel_text_start   : kernel_text_start   as *const u8,
            kernel_text_end     : kernel_text_end     as *const u8,
            kernel_bss_start    : kernel_bss_start    as *const u8,
            kernel_bss_end      : kernel_bss_end      as *const u8,
        };

        devices::init_devices(&mut jerry_meta_data);
    }

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // TODO(chungmcl): Implement panic handler
    loop {}
}
