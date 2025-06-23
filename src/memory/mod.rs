pub mod ttd;
pub mod ppm;
pub mod ptm;
pub use crate::types::*;
pub use core::{ptr, arch::asm};
pub use modular_bitfield::{*, specifiers::*};
pub use ttd::*;
use ppm::*;
use ptm::*;

pub enum MemorySetupError {
    PPMInitFailed(PPMError),
    KernelPTBootStrapFailed(PTMError)
}
 
pub fn init_memory(
    static_kernel_mem_end: *const u8, 
    ram_start: *const u8, 
    ram_len: usize
) -> Result<(), MemorySetupError> {
    let kernel_mem_end: *const u8;
    match init_ppm(static_kernel_mem_end, ram_start, ram_len) {
        Ok(static_kernel_mem_plus_phys_page_registry) => { 
            kernel_mem_end = static_kernel_mem_plus_phys_page_registry;
        },
        Err(e) => {
            return Err(MemorySetupError::PPMInitFailed(e));
        }
    }

    if let Err(e) = bootstrap_kernel_page_tables(ram_start, ram_len, kernel_mem_end) {
        return Err(MemorySetupError::KernelPTBootStrapFailed(e));
    }

    enable_mmu(
        (&raw const KERNEL_ROOT_TABLE0) as *const TableDescriptorS1, 
        (&raw const KERNEL_ROOT_TABLE1) as *const TableDescriptorS1, 
        TcrEl1::new()
            .with_tg0(0b10)
            .with_tg1(0b01)
            .with_ds(false)
            .with_t0sz(25)
            .with_t1sz(25)
    );

    Ok(())
}

#[inline(always)]
fn enable_mmu(ttbr0_el1: *const TableDescriptorS1, ttbr1_el1: *const TableDescriptorS1, tcr_el1: TcrEl1) {
    unsafe {
        asm!(
            "msr ttbr0_el1, {br0}", // Set TTBR0.
            "msr ttbr1_el1, {br1}", // Set TTBR1.
            "msr tcr_el1, {tcr}",   // Set TCR.
            "isb",                  // The ISB forces these changes to be seen before the MMU is enabled.
            "mrs {tmp}, sctlr_el1", // Read System Control Register configuration data.
            "orr {tmp}, {tmp}, #1", // Set [M] bit and enable the MMU.
            "msr sctlr_el1, {tmp}", // Write System Control Register configuration data.
            "isb",                  // The ISB forces these changes to be seen by the next instruction.
            tmp = lateout(reg) _,
            br0 = in(reg) ttbr0_el1,
            br1 = in(reg) ttbr1_el1,
            tcr = in(reg) u64::from_le_bytes(tcr_el1.into_bytes()),
            options(nostack, preserves_flags),
        );
    }
}

#[bitfield(bits = 64)]
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct TcrEl1 {
    pub t0sz        : B6,
    pub res0_6      : bool,
    pub epd0        : bool,
    pub irgn0       : B2,
    pub orgn0       : B2,
    pub sh0         : B2,
    pub tg0         : B2,
    pub t1sz        : B6,
    pub a1          : bool,
    pub epd1        : bool,
    pub irgn1       : B2,
    pub orgn1       : B2,
    pub sh1         : B2,
    pub tg1         : B2,
    pub ips         : B3,
    pub res0_35     : bool,
    pub as0         : bool,
    pub tbi0        : bool,
    pub tbi1        : bool,
    pub ha          : bool,
    pub hd          : bool,
    pub hpd0        : bool,
    pub hpd1        : bool,
    pub hwu059      : bool,
    pub hwu060      : bool,
    pub hwu061      : bool,
    pub hwu062      : bool,
    pub hwu159      : bool,
    pub hwu160      : bool,
    pub hwu161      : bool,
    pub hwu162      : bool,
    pub tbid0       : bool,
    pub tbid1       : bool,
    pub nfd0        : bool,
    pub nfd1        : bool,
    pub e0pd0       : bool,
    pub e0pd1       : bool,
    pub tcma0       : bool,
    pub tcma1       : bool,
    pub ds          : bool,
    pub mtx0        : bool,
    pub mtx1        : bool,
    pub res0_63_62  : B2,
}