use crate::memory::*;
use ttd::*;

pub enum PTMError {
    GetFreePageFailed(PPMError),
    MapPageToVAFAiled(PPMError),
    VAAlreadyMapped
}

#[unsafe(link_section = ".kernel_root_tables")] #[unsafe(no_mangle)]
pub static mut KERNEL_ROOT_TABLE0: [TableDescriptorS1; 8] = [TableDescriptorS1::new(); 8];

#[unsafe(link_section = ".kernel_root_tables")] #[unsafe(no_mangle)]
pub static mut KERNEL_ROOT_TABLE1: [TableDescriptorS1; 8] = [TableDescriptorS1::new(); 8];

pub fn bootstrap_kernel_page_tables(kernel_mem_end: *const u8) -> Result<(), PTMError> {
    /*
     * Boot👢strap🔫 the kernel VA by identity mapping the initial
     * kernel page table, i.e. mapping PA == VA. This is necessary because
     * after we turn the MMU on in enableMMU(), the processor will start
     * using the MMU to translate all referenced memory addresses via the page tables
     * pointed to by $TTBR0_EL1 and $TTBR1_EL1. This will also include the addresses
     * in $PC and $SP, meaning that after enableMMU() finishes and the MMU is on, the
     * processor will look into the page tables for the translation of ($PC + 4). Therefore,
     * we need to map the kernel's physical pages to a VA that is equal to its physical page's
     * PA such that the MMU is able to translate the VA ($PC + 4) to the PA ($PC + 4).
     * We setup the page kernel page table here to do this identity mapping before we call
     * enableMMU().
     * 
     * Note that the Kernel's addy range should not be in the "high" range of the VA, i.e.
     * the VA range mapped by $TTBR1_EL1, so we don't do anything to kernelRootTable1.
    */
    unsafe {
        let mut cur_page: *const u8 = 0 as *const u8;
        while cur_page < kernel_mem_end.sub(1) {
            match map_page_to_va(
                &mut *(&raw mut KERNEL_ROOT_TABLE0), 
                cur_page, 
                cur_page, 
                false
            ) {
                Ok(mapped_pa) => {
                    if mapped_pa != cur_page {
                        return Err(PTMError::VAAlreadyMapped);
                    }
                },
                Err(e) => {
                    return Err(e);
                }
            }
            cur_page = cur_page.add(PAGE_LEN);
        }
        return Ok(());
    }
}

pub fn map_page_to_va(
    root_table: &mut [TableDescriptorS1; 8], 
    page_pa: *const u8, 
    va: *const u8, 
    overwrite: bool
) -> Result<*const u8, PTMError> {
    let l1_idx: usize = get_bits(va as u64, 38, 36) as usize;
    let l2_idx: usize = get_bits(va as u64, 35, 25) as usize;
    let l3_idx: usize = get_bits(va as u64, 24, 14) as usize;

    let l2_table: &mut [TableDescriptorS1; 2048];
    let l3_table: &mut [PageDescriptorS1; 2048];
    unsafe {
        if root_table[l1_idx].valid_bit() && root_table[l1_idx].table_descriptor() {
            l2_table = &mut *(nlta_to_pa(root_table[l1_idx].nlta() as u64) as *mut [TableDescriptorS1; 2048]);
        } else {
            match get_free_page(true) {
                Ok(l2_table_ptr) => {
                    root_table[l1_idx] = TableDescriptorS1::new()
                        .with_valid_bit(true)
                        .with_table_descriptor(true)
                        .with_nlta(pa_to_nlta(l2_table_ptr))
                    ;
                    l2_table = &mut *(l2_table_ptr as *mut [TableDescriptorS1; 2048]);
                },
                Err(e) => {
                    return Err(PTMError::GetFreePageFailed(e));
                }
            }
        }

        if l2_table[l2_idx].valid_bit() && l2_table[l2_idx].table_descriptor() {
            l3_table = &mut *(nlta_to_pa(l2_table[l2_idx].nlta() as u64) as *mut [PageDescriptorS1; 2048]);
        } else {
            match get_free_page(true) {
                Ok(l3_table_ptr) => {
                    l2_table[l2_idx] = TableDescriptorS1::new()
                        .with_valid_bit(true)
                        .with_table_descriptor(true)
                        .with_nlta(pa_to_nlta(l3_table_ptr))
                    ;
                    l3_table = &mut *(l3_table_ptr as *mut [PageDescriptorS1; 2048]);
                },
                Err(e) => {
                    return Err(PTMError::GetFreePageFailed(e));
                }
            }
        }

        if l3_table[l3_idx].valid_bit() && l3_table[l3_idx].descriptor_type() && !overwrite {
            return Ok(oab_to_pa(l3_table[l3_idx].oab() as u64));
        } else {
            l3_table[l3_idx] = PageDescriptorS1::new()
                .with_valid_bit(true)
                .with_descriptor_type(true)
                .with_oab(pa_to_oab(page_pa))
                .with_af(true)
            ;
            return Ok(page_pa);
        }
    }
}
