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

    /*
     * When we calculate pages_to_map, we have to calculate pages_needed_for_page_table
     * and add it to pages_to_map, since we need the table ITSELF mapped into VA space.
     * Example of what we eventually want to happen:
     * # In this example, 0  is the highest allocated l1 table index,
     * #                  32 is the highest allocated l2 table index,
     * #                  66 is the highest allocated l3 table index.
     * | *((uint64_t*)(*((uint64_t*)&KERNEL_ROOT_TABLE0[0]) & ~0x03) + 32) & ~0x03
     * |  = 0x0000000040108000 # address of highest allocated l3 table
     * 
     * | *(((uint64_t*)(*((uint64_t*)((*((uint64_t*)&KERNEL_ROOT_TABLE0[0])) & ~0x03) + 32) & ~0x03)) + 66) & ~0x403
     * |  = 0x0000000040108000 # address of the highest allocated physical page
     * # We've converged the VA space described by the page table tree, with the PA space INCLUDING
     * # the page table tree itself!!!
     * 
     * Expanded out:
     * | *(
     * |     (uint64_t*)(
     * |         *(
     * |             (uint64_t*)&KERNEL_ROOT_TABLE0[0]         // Address of the first element of KERNEL_ROOT_TABLE0
     * |         )                                             // Get the first element of KERNEL_ROOT_TABLE0
     * |         & ~(0x03)                                     // Ignore TableDescriptorS1.{valid_bit, descriptor_type}
     * |     )                                                 // Cast first element of KERNEL_ROOT_TABLE0 to u64 pointer; points to first L2 Table
     * |     + 32                                              // Address of the 32nd element of the L2 Table (32 is the last used element of the L2 Table)
     * | )                                                     // Get the 32nd element of the L2 Table
     * | & ~(0x03)                                             // Ignore TableDescriptorS1.{valid_bit, descriptor_type}
     * | = 0x0000000040108000                                  // Pointer to last L3 Table
     * 
     * | *(
     * |     (uint64_t*)(
     * |         *(
     * |             (uint64_t*)(
     * |                 *(
     * |                     (uint64_t*)&KERNEL_ROOT_TABLE0[0] // Address of the first element of KERNEL_ROOT_TABLE0
     * |                 )                                     // Get the first element of KERNEL_ROOT_TABLE0
     * |                 & ~(0x03)                             // Ignore TableDescriptorS1.{valid_bit, descriptor_type}
     * |             )                                         // Cast first element of KERNEL_ROOT_TABLE0 to u64 pointer; points to first L2 Table
     * |             + 32                                      // Address of the 32nd element of the L2 Table (32 is the last used element of the L2 Table)
     * |         )                                             // Get the 32nd element of the L2 Table
     * |         & ~(0x03)                                     // Ignore TableDescriptorS1.{valid_bit, descriptor_type}                       
     * |     )                                                 // Cast 32nd element of L2 Table to u64 pointer; points to first L3 Table below 32nd L2 Table              
     * |     + 66                                              // Address of the 66th element of the L3 Table (66 is the last used element of the L3 Table)                  
     * | )                                                     // Get the 66th element of the L3 Table                
     * | & ~(0x403)                                            // Ignore PageDescriptorS1.{valid_bit, descriptor_type, af}                        
     * | = 0x0000000040108000                                  // Pointer to last physical page that's been used
     * 
     * So in other words, the last L3 Table in the page table tree is also the last physical page that's been used!
     * Therefore, if we setup the page table such that the above is true, we'll have mapped all memory that's been 
     * used by the kernel thus far (including the page table tree itself) into the page table tree. We would then be
     * finally Boot👢strap🔫ped and ready to enable the MMU.                                    
    */

    unsafe {
        let kernel_mem_pages: u64 = ((kernel_mem_end as u64 - 1) / PAGE_LEN as u64) + 1;

        let kernel_mem_end_l1_idx: u64 = get_bits(kernel_mem_end as u64, 46, 36);
        let kernel_mem_end_l2_idx: u64 = get_bits(kernel_mem_end as u64, 35, 25);
        let kernel_mem_end_l3_idx: u64 = get_bits(kernel_mem_end as u64, 24, 14);
        if (kernel_mem_end_l1_idx >= 7)    || // TODO(chungmcl): verify/implement fix for this edge case:
           (kernel_mem_end_l2_idx >= 2047) || // I think technically if l2_idx or l3_idx is >= 2047
           (kernel_mem_end_l3_idx >= 2047) {  // I can just += 1 to pages_needed_for_page_table...?
            panic!("");
        }
        let pages_needed_for_page_table: u64 =
            kernel_mem_end_l1_idx + 1 // #(l2 tables allocated via get_page())
            + 
            kernel_mem_end_l2_idx + 1 // #(l3 tables allocated via get_page())
            // l3 tables' entries are pointers to physical pages, so don't include in calculation
        ;

        let pages_to_map: u64 = kernel_mem_pages + pages_needed_for_page_table;
        for pa in (0..pages_to_map * PAGE_LEN as u64).step_by(PAGE_LEN) {
            let cur_page: *const u8 = pa as *const u8;
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
