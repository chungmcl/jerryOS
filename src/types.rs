
pub struct JerryMetaData {
    pub kernel_dtb_start    : *const u8,
    pub kernel_dtb_end      : *const u8,
    pub kernel_init_sp      : *const u8,
    pub kernel_bin_start    : *const u8,
    pub kernel_rodata_start : *const u8,
    pub kernel_rodata_end   : *const u8,
    pub kernel_text_start   : *const u8,
    pub kernel_text_end     : *const u8,
    pub kernel_bss_start    : *const u8,
    pub kernel_bss_end      : *const u8,
}
