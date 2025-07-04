use crate::devices::FDTError;
pub use crate::types::*;
pub mod ttd;
pub mod ppm;
pub mod ptm;
pub use core::{ptr, arch::asm};
pub use modular_bitfield::{*, specifiers::*};
use super::*;
pub use ttd::*;
use ppm::*;
use ptm::*;

pub enum MemoryError {
    RAMFDTNodeGetRegFailed(FDTError),
    PPMInitFailed(PPMError),
    KernelPTBootStrapFailed(PTMError)
}
 
pub fn init_memory(
    dram_node: FDTNode,
    static_kernel_mem_end: *const u8,
    dtb_start: *const u8,
    dtb_end: *const u8
) -> Result<(), MemoryError> {
    let ram_start: *const u8;
    let ram_len: usize;

    match dram_node.get_reg() {
        Ok((specified_ram_start, specified_ram_len)) => {
            ram_start = specified_ram_start as *const u8;
            ram_len = specified_ram_len as usize;
            unsafe { RAM_START = ram_start; RAM_LEN = ram_len; }
        },
        Err(e) => { return Err(MemoryError::RAMFDTNodeGetRegFailed(e)); }
    }

    let kernel_mem_end: *const u8;
    match init_ppm(static_kernel_mem_end, ram_start, ram_len) {
        Ok(static_kernel_mem_plus_phys_page_registry) => { 
            kernel_mem_end = static_kernel_mem_plus_phys_page_registry;
        },
        Err(e) => {
            return Err(MemoryError::PPMInitFailed(e));
        }
    }

    if let Err(e) = bootstrap_kernel_page_tables(
        dtb_start,
        dtb_end,
        ram_start, 
        ram_len, 
        kernel_mem_end
    ) {
        return Err(MemoryError::KernelPTBootStrapFailed(e));
    }

    enable_mmu(
        get_kernel_root_table_0() as *const TableDescriptorS1, 
        get_kernel_root_table_1() as *const TableDescriptorS1, 
        TcrEl1::new()
            .with_tg0(0b10)
            .with_tg1(0b01)
            .with_ds(false)
            .with_t0sz(T0_T1_SZ as u8)
            .with_t1sz(T0_T1_SZ as u8)
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
        MMU_ENABLED = true;
    }
}

static mut RAM_START: *const u8 = ptr::null();
static mut RAM_LEN: usize = 0;
static mut MMU_ENABLED: bool = false;
#[inline(always)] pub fn mmu_is_enabled() -> bool { unsafe { MMU_ENABLED } }

// 2¹⁴ -> 16KB
const PAGE_GRANULARITY: usize = 14;
const PAGE_LEN: usize = 1 << PAGE_GRANULARITY;

// The size offset of the memory region addressed by $TTBR0_EL1/$TTBR1_EL1. The region size is 2⁽⁶⁴⁻ᵀ⁰-ᵀ¹-ˢz⁾ bytes.
// i.e. the TTBRs' VA addresses use 64 - T0_T1_SZ = 38 bits. 
// This also means to access TTBR1's VA space, one must set the top T0_T1_SZ bits to 0b1.
const T0_T1_SZ: usize = 25;
const TTBR1_MASK: usize = n_bits(T0_T1_SZ) << (usize::BITS as usize - T0_T1_SZ);
#[inline(always)] pub fn ram_va_to_pa(va: usize) -> usize { unsafe { (!TTBR1_MASK &  va) + RAM_START as usize  } }
#[inline(always)] pub fn pa_to_ram_va(pa: usize) -> usize { unsafe {   TTBR1_MASK | (pa  - RAM_START as usize) } }

const TABLE_ENTRY_LEN:  usize = 1 <<  3;
const L1_TABLE_ENTRIES: usize = 1 <<  3;
const L2_TABLE_ENTRIES: usize = 1 << 11;
const L3_TABLE_ENTRIES: usize = 1 << 11;

// (usize, usize) == [MSB:LSB]
const L1_SELECT_BITS_RANGE: (usize, usize) = (38, 36);
const L2_SELECT_BITS_RANGE: (usize, usize) = (35, 25);
const L3_SELECT_BITS_RANGE: (usize, usize) = (24, 14);

type L1Table = [TableDescriptorS1; L1_TABLE_ENTRIES];
type L2Table = [TableDescriptorS1; L2_TABLE_ENTRIES];
type L3Table = [PageDescriptorS1;  L3_TABLE_ENTRIES];

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