#![no_std]
#![no_main]
use core::panic::PanicInfo;

#[unsafe(no_mangle)]
pub extern "C" fn main() -> ! {
    let a: u32 = 1;
    let b: u32 = 2;
    let _c: u32 = a + b;
    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        // You can add e.g. logging or a halt instruction here if you want
    }
}

#[link(name = "entry")]
unsafe extern "C" {}