#![no_std]
#![no_main]
#![allow(dead_code)]
#[link(name = "entry")] unsafe extern "C" {}
use core::panic::PanicInfo;
pub use core::arch::asm;
pub use core::ptr::*;
pub use crate::types::*;
mod types;
mod devices;

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
            kernel_dtb_end      : 0                   as *const u8, // Set at run-time in devices::init_devices()
            kernel_init_sp      : kernel_init_sp      as *const u8,
            kernel_bin_start    : kernel_bin_start    as *const u8,
            kernel_rodata_start : kernel_rodata_start as *const u8,
            kernel_rodata_end   : kernel_rodata_end   as *const u8,
            kernel_text_start   : kernel_text_start   as *const u8,
            kernel_text_end     : kernel_text_end     as *const u8,
            kernel_bss_start    : kernel_bss_start    as *const u8,
            kernel_bss_end      : kernel_bss_end      as *const u8,
            ram_start           : 0                   as *const u8, // Set at run-time in devices::init_devices()
            ram_len             : 0                                 // Set at run-time in devices::init_devices()
        };

        match devices::init_devices(jerry_meta_data.kernel_dtb_start) { 
            Ok((kernel_dtb_end, ram_start, ram_len)) => {
                jerry_meta_data.kernel_dtb_end = kernel_dtb_end;
                jerry_meta_data.ram_start = ram_start;
                jerry_meta_data.ram_len = ram_len;
            },
            Err(_e) => {
                panic!("devices::init_devices errored!") 
            }
        }
    }

    loop {}
}

#[panic_handler]
fn jerry_panic(info: &PanicInfo) -> ! {
    let _msg: &str = info
        .message()
        .as_str()
        .expect("Unknown panic!")
    ;
    /*
        p/s _msg.data_ptr
    */

    loop {}
}
