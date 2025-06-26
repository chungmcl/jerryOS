use super::*;
use core::fmt;

const DATA_REG_OFFSET: usize = 0x0;
const FLAG_REG_OFFSET: usize = 0x18;
const TX_FIFO_FULL_MASK: u32 = 1 << 5;

static mut ADDRESS: *const u8 = ptr::null();
static mut DATA_REG: *mut u32 = ptr::null_mut();
static mut FLAG_REG: *const u32 = ptr::null();
static mut PL011_INITIALIZED: bool = false;

pub enum PL011Error {
    GetRegsFailed(FDTError),
    MapMMIORangeFailed(PTMError)
}

pub fn init_pl011_uart(pl011_node: FDTNode) -> Result<*const u8, PL011Error> {
    unsafe {
        ADDRESS = match pl011_node.get_reg() {
            Ok((pl011_mmio_address, pl011_mmio_size)) => {
                if let Err(e) = map_mmio_range(
                    pl011_mmio_address as *const u8, 
                    pl011_mmio_size as usize
                ) { return Err(PL011Error::MapMMIORangeFailed(e)); }
                pl011_mmio_address as *const u8
            },
            Err(e) => {
                return Err(PL011Error::GetRegsFailed(e));
            }
        };
        DATA_REG = ADDRESS.add(DATA_REG_OFFSET) as *mut u32;
        FLAG_REG = ADDRESS.add(FLAG_REG_OFFSET) as *const u32;
        PL011_INITIALIZED = true;
        return Ok(ADDRESS);
    }
}

fn write(b: u8) {
    unsafe {
        while read32_ptr(FLAG_REG) & TX_FIFO_FULL_MASK != 0 {}
        write32_ptr(DATA_REG, b as u32);
    }
}

fn write_str(s: &str) {
    for &b in s.as_bytes() {
        if b == b'\n' { write(b'\r'); }
        write(b);
    }
}

pub struct PL011Writer;

impl fmt::Write for PL011Writer {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        write_str(s);
        return Ok(());
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ({
        let _ = core::fmt::write(
            &mut $crate::PL011Writer, 
            format_args!($($arg)*)
        );
    });
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($fmt:expr) => (
        $crate::print!(
            concat!($fmt, "\n")
        )
    );
    ($fmt:expr, $($arg:tt)*) => (
        $crate::print!(
            concat!($fmt, "\n"), 
            $($arg)*
        )
    );
}
