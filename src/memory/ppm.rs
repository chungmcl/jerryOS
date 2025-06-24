use crate::memory::*;

static mut PHYS_PAGE_REGISTRY: *mut u8 = ptr::null_mut();
static mut PHYS_PAGE_REGISTRY_LEN: usize = 0;

pub enum PPMError {
    PageIdxOutOfRange,
    PageHasNoReferences,
    PageHasMaxReferences,
    InvalidPageIdxRange,
    ExpectedFreePageHasReferences,
    NoFreePages
}

pub fn init_ppm(static_kernel_mem_end: *const u8, ram_start: *const u8, ram_len: usize) -> Result<*const u8, PPMError> {
    unsafe {
        // .sub(1) since ..._end points to first byte after last byte of data
        // +1 because index -> count
        let static_kernel_mem_pages: usize = pa_to_page_idx(static_kernel_mem_end.sub(1)) + 1;
        PHYS_PAGE_REGISTRY = page_idx_to_pa_mut(static_kernel_mem_pages);
        // ram_start for #(physical addresses from 0x0 to start of RAM).
        PHYS_PAGE_REGISTRY_LEN = (ram_start.add(ram_len) as usize) / PAGE_LEN;
        ptr::write_bytes(PHYS_PAGE_REGISTRY, 0x00, PHYS_PAGE_REGISTRY_LEN);
        let phys_page_registry_pages: usize = (PHYS_PAGE_REGISTRY_LEN / PAGE_LEN) + 1;
        let already_used_pages: usize = static_kernel_mem_pages + phys_page_registry_pages;
        match increment_ref_count_range(already_used_pages - 1, 0) {
            Ok(_) => { 
                return Ok(page_idx_to_pa(already_used_pages));
            },
            Err(e) => {
                return Err(e);
            }
        }
    }
}

pub fn get_num_phys_pages() -> usize {
    unsafe {
        return PHYS_PAGE_REGISTRY_LEN;
    }
}

pub fn get_free_page(zero_out: bool) -> Result<*const u8, PPMError> {
    unsafe {
        for i in 0..PHYS_PAGE_REGISTRY_LEN {
            if *PHYS_PAGE_REGISTRY.add(i) == 0 {
                match increment_ref_count(i) {
                    Ok(ref_count) => {
                        if ref_count != 0x01 {
                            return Err(PPMError::ExpectedFreePageHasReferences);
                        }
                    },
                    Err(e) => {
                        return Err(e);
                    }
                };
                // TODO(chungmcl): After the MMU is enabled, must map and/or get
                // the kernel VA of the PA.
                let page_pa: *mut u8 = page_idx_to_pa_mut(i);
                if zero_out { ptr::write_bytes(page_pa, 0x00, PAGE_LEN); }
                return Ok(page_pa);
            }
        }
        return Err(PPMError::NoFreePages);
    }
}

pub fn get_page(page_idx: usize) -> Result<*const u8, PPMError> {
    unsafe {
        if *PHYS_PAGE_REGISTRY.add(page_idx) >= u8::MAX {
            return Err(PPMError::PageHasMaxReferences);
        } else {
            return Ok(page_idx_to_pa(page_idx));
        }
    }
}

fn free_page_ref(page_ref: *const u8) -> Result<u8, PPMError> {
    return decrement_ref_count(pa_to_page_idx(page_ref));
}

fn increment_ref_count(page_idx: usize) -> Result<u8, PPMError> {
    unsafe {
        if page_idx >= PHYS_PAGE_REGISTRY_LEN {
            return Err(PPMError::PageIdxOutOfRange);
        }

        let page_ref_counter: *mut u8 = PHYS_PAGE_REGISTRY.add(page_idx);
        let cur_ref_count: u8 = *page_ref_counter;
        if cur_ref_count >= u8::MAX {
            return Err(PPMError::PageHasMaxReferences);
        }

        *page_ref_counter += 1;
        return Ok(*page_ref_counter);
    }
}

fn increment_ref_count_range(high_idx: usize, low_idx: usize) -> Result<(), PPMError> {
    unsafe {
        if (high_idx <= low_idx) | (high_idx >= PHYS_PAGE_REGISTRY_LEN) {
            return Err(PPMError::InvalidPageIdxRange);
        }

        for page_idx in low_idx..=high_idx {
            let page_ref_counter: *mut u8 = PHYS_PAGE_REGISTRY.add(page_idx);
            let cur_ref_count: u8 = *page_ref_counter;
            if cur_ref_count >= u8::MAX {
                return Err(PPMError::PageHasMaxReferences);
            }
        }

        for page_idx in low_idx..=high_idx {
            let page_ref_counter: *mut u8 = PHYS_PAGE_REGISTRY.add(page_idx);
            *page_ref_counter += 1;
        }

        return Ok(());
    }
}

fn decrement_ref_count(page_idx: usize) -> Result<u8, PPMError> {
    unsafe {
        if page_idx >= PHYS_PAGE_REGISTRY_LEN {
            return Err(PPMError::PageIdxOutOfRange);
        }

        let page_ref_counter: *mut u8 = PHYS_PAGE_REGISTRY.add(page_idx);
        let cur_ref_count: u8 = *page_ref_counter;
        if cur_ref_count <= 0x00 {
            return Err(PPMError::PageHasNoReferences);
        }

        *page_ref_counter -= 1;
        return Ok(*page_ref_counter);
    }
}

fn decrement_ref_count_range(high_idx: usize, low_idx: usize) -> Result<(), PPMError> {
    unsafe {
        if (low_idx <= high_idx) | (high_idx >= PHYS_PAGE_REGISTRY_LEN) {
            return Err(PPMError::InvalidPageIdxRange);
        }

        for page_idx in low_idx..=high_idx {
            let page_ref_counter: *mut u8 = PHYS_PAGE_REGISTRY.add(page_idx);
            let cur_ref_count: u8 = *page_ref_counter;
            if cur_ref_count <= 0x00 {
                return Err(PPMError::PageHasNoReferences);
            }
        }

        for page_idx in low_idx..=high_idx {
            let page_ref_counter: *mut u8 = PHYS_PAGE_REGISTRY.add(page_idx);
            *page_ref_counter -= 1;
        }

        return Ok(());
    }
}

#[inline(always)]
pub fn page_idx_to_pa(page_idx: usize) -> *const u8 {
    return (page_idx * PAGE_LEN) as *const u8;
}

#[inline(always)]
pub fn pa_to_page_idx(pa: *const u8) -> usize {
    return (pa as usize) / PAGE_LEN;
}

#[inline(always)]
pub fn page_idx_to_pa_mut(page_idx: usize) -> *mut u8 {
    return (page_idx * PAGE_LEN) as *mut u8;
}

#[inline(always)]
pub fn pa_to_page_idx_mut(pa: *mut u8) -> usize {
    return (pa as usize) / PAGE_LEN;
}
