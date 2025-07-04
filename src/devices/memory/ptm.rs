use super::*;

pub enum PTMError {
    GetFreePageFailed(PPMError),
    MapPageToVAFAiled(PPMError),
    VAAlreadyMapped
}

#[unsafe(link_section = ".kernel_root_tables")] #[unsafe(no_mangle)]
static mut KERNEL_ROOT_TABLE0: L1Table = [TableDescriptorS1::new(); L1_TABLE_ENTRIES];
#[inline(always)] pub fn get_kernel_root_table_0() -> &'static mut L1Table { unsafe { &mut *(&raw mut KERNEL_ROOT_TABLE0) } }

#[unsafe(link_section = ".kernel_root_tables")] #[unsafe(no_mangle)]
static mut KERNEL_ROOT_TABLE1: L1Table = [TableDescriptorS1::new(); L1_TABLE_ENTRIES];
#[inline(always)] pub fn get_kernel_root_table_1() -> &'static mut L1Table { unsafe { &mut *(&raw mut KERNEL_ROOT_TABLE1) } }

pub fn bootstrap_kernel_page_tables(
    dtb_start: *const u8,
    dtb_end: *const u8,
    ram_start: *const u8, 
    ram_len: usize, 
    kernel_mem_end: *const u8
) -> Result<(), PTMError> {
    unsafe {
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
         * Notice that calls to map_page_to_va() here cause more physical pages to be allocated
         * for the pages of the page table ITSELF. We don't map these pages into the $TTBR0_EL1 VA
         * space, which means if we were to enable the MMU at this point we wouldn't be able to make
         * any changes to the kernel's VA space. That's definitely an ability we'll need -- we solve this
         * conundrum in the next step. 
        */
        for pa in (dtb_start as usize..dtb_end as usize).step_by(PAGE_LEN) {
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
        for pa in (ram_start as usize..kernel_mem_end as usize).step_by(PAGE_LEN) {
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

       /*
         * We now map the entirety of RAM PA space into kernel $TTBR1_EL1 VA space.
         * If we map everything in RAM to SOME VA range for the kernel, it'll always
         * be able to access all parts of memory, which is something the kernel will 
         * need as the memory manager for user-level processes, and, as mentioned above, 
         * something the kernel needs to be able to modify ITS OWN VA space/page table tree.
         * 
         * 
         * Ostensibly, it may seem that it would waste a lot of pages to map the entirety
         * of RAM into VA space. However, if we do the math, we see that mapping all of RAM
         * PA space only uses a very small percentage (less than 1%) of available memory to
         * allocate the physical pages for page table that maps RAM PA space -> VA space:
         * 
         * For reference:
         * KB = 2¹⁰ == 1,024         bytes
         * MB = 2²⁰ == 1,048,576     bytes
         * GB = 2³⁰ == 1,073,741,824 bytes
         * 
         * Jerry uses a 16KB page granularity, which is 2¹⁴ bytes per page.
         * Each node* in the page table tree is also itself a 16KB page, and is an array of u64 pointers.
         *         (*with the exception of the root nodes (aka L1 nodes) in jerry, which is only an array of 8 u64s)
         * Therefore, a given page table node can hold 2¹⁴ ÷ 8 == 2¹¹ == 2048 u64 pointers, aka entries.
         * 
         * Each page table entry (PTE) in an L3 node points to a 16KB physical page.
         * If one L3 node can map 2048 16KB physical pages, that means an L3 node can map:
         *   2048 × 16KB == 2¹¹ × 2¹⁴ == 2²⁵ bytes == 32MB of memory.
         * 
         * Each translation table entry (TTE) in an L2 node points to a 16KB L3 node.
         * If one L2 node can map 2048 16KB L3 nodes, and each L3 node maps 32MB == 2²⁵ bytes of memory,
         * that means an L2 node can map:
         *   2048 × 2²⁵ == 2¹¹ × 2²⁵ == 2³⁶ bytes == 64GB of memory.
         * 
         * Each translation table entry (TTE) in an L1 node points to a 16KB L2 node.
         * If one L1 node can map 8 16KB L2 nodes, and each L2 node maps 64GB == 2³⁶ bytes of memory, 
         * that means an L1 node can map:
         *   8 × 2³⁶ == 2³ × 2³⁶ = 2³⁹ bytes == 512GB of memory.
         *  
         * We can observe that if we had for instance, 64GB of RAM we 
         * wanted to map, it would only take 
         *   1 L1 node, 1 L2 node, and 2048 L3 nodes, totaling for 64B + 16KB + 2048*16KB = 0.03125GB of pages table node pages.
         * As a percentage of 64GB, that's approximately only 0.05% of RAM!
         * 
         * Using our calculations above, we can generalize a formula of #(page table pages to map m bytes of memory), assuming:
         * • 64B L1 Node
         * • 16KB L2 & L3 Node
         * • u64/8-byte node entries
         * • 16KB physical pages
         * 
         * pagesToMap(m: bytes) := ceil(m ÷ 2³⁹) + ceil(m ÷ 2³⁶) + ceil(m ÷ 2²⁵)
         * 
         * We can see that #(page table pages to map m bytes of memory) grows linearly (almost, because of the ceil) as m changes,
         * which means we should always have approximately the same proportion of page table pages used to map m amount of RAM.
         * That proportion is approximately only ≅0.05% of total RAM PA space!!!
         * Also, note that:
         * • There are 2048 u8/8-byte PTE/TTEs in an L1/L2/L3 node, each of which point to a 16KB L2/L3 node or physical page.
         * • (One PTE/TTE) ÷ (2048 PTE/TTEs in an L1/L2/L3 node) = 8 ÷ (2048 × 8) = 1 ÷ 2048 ≅ 0.05%
         * 
         * Therefore, there is no practical concern about mapping all of RAM PA space into the kernel's VA space via page tables.
        */
        for pa in (ram_start as usize..(ram_start.add(ram_len)) as usize).step_by(PAGE_LEN) {
            let cur_page: *const u8 = pa as *const u8;
            match map_page_to_va(
                &mut *(&raw mut KERNEL_ROOT_TABLE1), 
                cur_page,
                cur_page.sub(ram_start as usize), // offset RAM VA to start at 0x00
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

// TODO(chungmcl): This currently just identity maps the MMIO physical addy for simplicity.
// How do I handle if a VA is already taken? Should a caller be able to specify a VA to map to?
pub fn map_mmio_range(
    mmio_address: *const u8,
    mmio_len: usize
) -> Result<(), PTMError> {
    unsafe {
        let mmio_pg_range_lo: *const u8 = page_idx_to_pa(pa_to_page_idx(mmio_address));
        let mmio_pg_range_hi: *const u8 = page_idx_to_pa(pa_to_page_idx(mmio_address.add(mmio_len)));

        for pa in (mmio_pg_range_lo as usize..=mmio_pg_range_hi as usize).step_by(PAGE_LEN) {
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

fn map_page_to_va(
    root_table: &mut L1Table, 
    page_pa: *const u8, 
    va: *const u8, 
    overwrite: bool
) -> Result<*const u8, PTMError> {
    let l1_idx: usize = get_bits(va as usize, L1_SELECT_BITS_RANGE.0, L1_SELECT_BITS_RANGE.1);
    let l2_idx: usize = get_bits(va as usize, L2_SELECT_BITS_RANGE.0, L2_SELECT_BITS_RANGE.1);
    let l3_idx: usize = get_bits(va as usize, L3_SELECT_BITS_RANGE.0, L3_SELECT_BITS_RANGE.1);

    let l2_table: &mut L2Table;
    let l3_table: &mut L3Table;
    unsafe {
        if root_table[l1_idx].valid_bit() && root_table[l1_idx].table_descriptor() {
            let mut l2_table_addy: *const u8 = nlta_to_pa(root_table[l1_idx].nlta() as u64);
            if mmu_is_enabled() { l2_table_addy = pa_to_ram_va(l2_table_addy as usize) as *const u8; }
            l2_table = &mut *(l2_table_addy as *mut L2Table);
        } else {
            match get_free_page(true) {
                Ok(l2_table_pa) => {
                    root_table[l1_idx] = TableDescriptorS1::new()
                        .with_valid_bit(true)
                        .with_table_descriptor(true)
                        .with_nlta(pa_to_nlta(l2_table_pa))
                    ;
                    let mut l3_table_addy: *const u8 = l2_table_pa;
                    if mmu_is_enabled() { l3_table_addy = pa_to_ram_va(l3_table_addy as usize) as *const u8; }
                    l2_table = &mut *(l3_table_addy as *mut L2Table);
                },
                Err(e) => {
                    return Err(PTMError::GetFreePageFailed(e));
                }
            }
        }

        if l2_table[l2_idx].valid_bit() && l2_table[l2_idx].table_descriptor() {
            let mut l3_table_addy: *const u8 = nlta_to_pa(l2_table[l2_idx].nlta() as u64);
            if mmu_is_enabled() { l3_table_addy = pa_to_ram_va(l3_table_addy as usize) as *const u8; }
            l3_table = &mut *(l3_table_addy as *mut L3Table);
        } else {
            match get_free_page(true) {
                Ok(l3_table_pa) => {
                    l2_table[l2_idx] = TableDescriptorS1::new()
                        .with_valid_bit(true)
                        .with_table_descriptor(true)
                        .with_nlta(pa_to_nlta(l3_table_pa))
                    ;
                    let mut l3_table_addy: *const u8 = l3_table_pa;
                    if mmu_is_enabled() { l3_table_addy = pa_to_ram_va(l3_table_addy as usize) as *const u8; }
                    l3_table = &mut *(l3_table_addy as *mut L3Table);
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
