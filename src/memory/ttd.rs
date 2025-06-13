use modular_bitfield::prelude::*;

/*
 * Notes for how to interpret what bits represent in TTEs via the A-Profile Reference:
 * 
 * 1. jerry will run on a Cortex-A710 processor.
 *    Cortex-A710 processor features (non-exhaustive, just some that relate to/affect TTE bits):
 *    Feature      Implemented?
 *    -----------  ------------
 *    FEAT_AIE     ❌ 
 *    FEAT_BBM     ✅
 *      • & ID_AA64MMFR2_EL1.BBM = 0b0010	
 *        -> Level 2 support for changing block size is supported
 *    FEAT_BTI     ✅
 *    FEAT_HAFDBS  ✅
 *      • -> Support for hardware update of the Access flag for Block & Page descriptors
 *      • & ID_AA64MMFR1_EL1.HAFDBS=0b0010 -> Support for hardware update of dirty state
 *    FEAT_HAFT    ❌            
 *      • -> NO support for hardware update of the Access flag for Table descriptors
 *    FEAT_HPDS2   ✅
 *    FEAT_LPA     ❌
 *    FEAT_LPA2    ❌            
 *      • -> TCR_EL{1,2}.DS=0 & RES0
 *      • -> VTCR_EL2=0 & RES0
 *    FEAT_LVA     ❌
 *    FEAT_NV      ❌
 *      • & (FEAT_NV2 not implemented) -> HCR_EL2.{NV, NV1}={0, 0} & {RES0,RES0}
 *    FEAT_NV2     ❌
 *      • & (FEAT_NV not implemented) -> HCR_EL2.{NV, NV1}={0, 0} & {RES0,RES0}
 *    FEAT_NV2p1   ❌
 *    FEAT_S2FWB   ✅
 *      • -> HCR_EL2.FWB is supported
 *      • -> if HCR_EL2.FWB=0 -> the combination of stage 1 and stage 2 translations 
 *           on memory type and cacheability attributes are as described in the 
 *           Armv8.0 architecture. For more information, see 'Combining stage 1 and 
 *           stage 2 memory type attributes'.
 *      • -> if HCR_EL2.FWB=1 -> the encoding of the stage 2 memory type and cacheability 
 *           attributes in bits[5:2] of the stage 2 Page or Block descriptors are as 
 *           described in 'Stage 2 memory type and Cacheability attributes when 
 *           FEAT_S2FWB is enabled'.
 *    FEAT_S1PIE   ❌
 *      • -> Stage 1 Direct permissions are used
 *      • -> Stage 1 Indirect permissions are disabled
 *    FEAT_S1POE   ❌
 *      • -> Stage 1 Overlay permissions are disabled
 *    FEAT_S2PIE   ❌
 *      • -> Stage 2 Direct permissions are used
 *      • -> Stage 2 Indirect permissions are disabled
 *    FEAT_S2POE   ❌
 *      • -> Stage 2 Overlay permissions are disabled
 *    FEAT_TCR2    ❌
 *      • -> TCR2_EL1, TCR2_EL2, and their associated trap controls are not implemented
 *      • note: Although the ARM ref. manual states "FEAT_TCR2 is mandatory from Armv8.9" (ARM DDI 0487, Page A2-142), and jerry is running on 
 *              a Cortex A710 which is Armv9.0, it is important to note that ARM states that "Arm®v9.0-A architecture extends the architecture 
 *              defined in the Armv8-A architectures up to Arm®v8.5-A." (https://developer.arm.com/documentation/101800/0201/The-Cortex-A710--core/Supported-standards-and-specifications) 
 *              In other words, Armv9.0 is not necessarily "greater than" Armv8.9; in reality it's Armv8.5 with some extra features slapped on top. 
 *              (Also see https://developer.arm.com/documentation/102378/0201/Armv8-x-and-Armv9-x-extensions-and-features)
 *              Therefore, it is expected that FEAT_TCR2 is not implemented on the Cortex A710.
 *    FEAT_THE     ❌
 *      • -> Effective value of PnCH=0 & RES0
 *      • -> Effective value of VTCR_EL2.AssuredOnly=0 & RES0
 *    FEAT_TTST    ✅
 *      • -> The maximum value of the TCR_ELx.{T0SZ,T1SZ} and VTCR_EL2.T0SZ fields is 48 for 4KB and 16KB granules, and 47 for 64KB granules.
 *    FEAT_XS      ❌
 *    FEAT_XNX     ✅
 *      • -> Distinction between EL0 and EL1 execute-never control at stage 2 supported
 * 
 * 2. jerry will use a 16KB translation granule.
 * 
 * 3. jerry will not use Secure state (no use of ARM Realms)
*/

/*
 * Some important definitions/info quoted directly from ARM docs:
 * • "Effective value: A register control field, meaning a field in a register that controls some aspect of the behavior, can be
 *    described as having an Effective value:
 *    • In some cases, the description of a control a specifies that when control a is active it causes a
 *      register control field b to be treated as having a fixed value for all purposes other than direct
 *      reads, or direct reads and direct writes, of the register containing control field b. When control
 *      a is active that fixed value is described as the Effective value of register control field b. For
 *      example, when the value of HCR.DC is 1, the Effective value of HCR.VM is 1, regardless of
 *      its actual value.
 *      In other cases, in some contexts a register control field b is not implemented or is not
 *      accessible, but behavior of the PE is as if control field b was implemented and accessible, and
 *      had a particular value. In this case, that value is the Effective value of register control field b.
 *      Note:
 *        Where a register control field is introduced in a particular version of the architecture, and is not
 *        implemented in an earlier version of the architecture, typically it will have an Effective value in
 *        that earlier version of the architecture.
 *    • Otherwise, the Effective value of a register control field is the value of that field."
 * • "Should Be Zero or Preserved (SBZP): The Large Physical Address Extension modifies the definition of SBZP for register bits that are reallocated by the extension, 
 *    and as a result are SBZP in some but not all contexts. The generic definition of SBZP given here applies only to bits that are not affected by this modification. 
 *    Hardware must ignore writes to the field. When writing this field, software must either write all 1s (sic) to this field, or, if the register is being restored from a 
 *    previously read state, this field must be written previously read value. If this is not done, then the result is UNPREDICTABLE. This description can 
 *    apply to a single bit that should be written as its preserved value or as 0, or to a field that should be written as its preserved value or as all 0s."
 * • "RES0: A reserved bit or field with Should-Be-Zero-or-Preserved (SBZP) behavior. Used for fields in register descriptions. Also used 
 *    for fields in architecturally defined data structures that are held in memory, for example in translation table descriptors. 
 *    RES0 is not used in descriptions of instruction encodings. Within the architecture, there are some cases where a register bit or bitfield 
 *    can be RES0 in some defined architectural context, but can have different defined behavior in a different architectural context. For any RES0 
 *    bit or field, software must not rely on the bit or field reading as 0, and must use an SBZP policy to write to the bit or field."
 * • "In the case of a 16kB granule, 
 *     • The hardware can use a 4-level look up process.
 *     • The 48-bit address has 11 address bits per level that is translated, that is 2048 entries each, with the final 14 bits selecting a byte 
 *       within the 4kB coming directly from the original address.
 *     • L0: 
 *        The level 0 table contains only two entries. Bit [47] of the virtual address selects a descriptor from the two entry L0 table. 
 *        Each of these table entries spans a 128TB range and points to an L1 table.
 *     • L1: 
 *        Within that 2048 entry L1 table, bits [46:36] are used as an index to select an entry and each entry points to an L2 table. 
 *        Bits [35:25] index into a 2048 entry L2 table and each entry points to a 32MB block or next table level.
 *     • L2: 
 *        At the final translation stage, bits [24:14] index into a 2048 entry L2 table and each entry points to a 16kB block.
 *     • ┌───────────────────────┐┌───────────────────────┐┌────────────────────────┐┌────────────────────────┐┌────────────────────────┐   
 *       │      VA bit [47]      ││    VA bit [46:36]     ││    VA bits [35:25]     ││    VA bits [24:14]     ││     VA bits [13:0]     │   
 *       └───────────────────────┘└───────────────────────┘└────────────────────────┘└────────────────────────┘└────────────────────────┘   
 *       ┌───────────────────────┐┌───────────────────────┐┌────────────────────────┐┌────────────────────────┐┌────────────────────────┐
 *       │  Level 0 Table Index  ││  Level 1 Table Index  ││   Level 2 Table Index  ││   Level 3 Table Index  ││                        │
 *       │ Each entry contains:  ││ Each entry contains:  ││ Each entry contains:   ││ Each entry contains:   ││                        │
 *       │ • Pointer to L1 table ││ • Pointer to L2 table ││ • Pointer to L3 table  ││                        ││      Block offset      │
 *       │ • (No block entry)    ││                       ││ • Base address of 32MB ││ • Base address of 16KB ││      and PA [13:0]     │
 *       │                       ││                       ││   block (IPA)          ││   block (IPA)          ││                        │
 *       └───────────────────────┘└───────────────────────┘└────────────────────────┘└────────────────────────┘└────────────────────────┘
 *   " (Note: jerryOS will choose to only use up to VA bit 38, such that we start at L1 and have 8 entries in the initial L1 table.)
*/

#[bitfield(bits = 64)]
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct TableDescriptorS1 {
    pub valid_bit        : bool,       
    pub table_descriptor : bool,
    pub ignored_7_2      : B6,       
    pub ignored_9_8      : B2,       
    pub ignored_10       : bool,      
    pub ignored_11       : bool,      
    pub res0_13_12       : B2,        
    pub nlta             : B34,             
    pub res0_49_48       : B2,        
    pub res0_50          : bool,         
    pub ignored_51       : bool,      
    pub ignored_52       : bool,      
    pub ignored_58_53    : B6,     
    pub pxn_table        : bool,       
    pub uxn_table        : bool,       
    pub ap_table         : B2,          
    pub res0_63          : bool,         
}

#[bitfield(bits = 64)]
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct PageDescriptorS1 {
    pub valid_bit        : bool,
    pub descriptor_type  : bool,
    pub attr_indx        : B3,  
    pub res0_5           : bool,
    pub ap               : B2,  
    pub shareability     : B2,  
    pub af               : bool,
    pub ng               : bool,
    pub res0_13_12       : B2,  
    pub oab              : B34, 
    pub res0_49_48       : B2,  
    pub gp               : bool,
    pub dbm              : bool,
    pub contiguous       : bool,
    pub pxn              : bool,
    pub uxn              : bool,
    pub ignored_55       : bool,
    pub res_sw_use_58_56 : B3,  
    pub pbha             : B4,  
    pub ignored_63       : bool,
}

#[bitfield(bits = 64)]
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct BlockDescriptorS1 {
    pub valid_bit        : bool,
    pub descriptor_type  : bool,
    pub attr_indx        : B3,  
    pub res0_5           : bool,
    pub ap               : B2,  
    pub shareability     : B2,  
    pub af               : bool,
    pub ng               : bool,
    pub res0_15_12       : B4,  
    pub nt               : bool,
    pub res0_24_17       : B8,  
    pub oab              : B23, 
    pub res0_49_48       : B2,  
    pub gp               : bool,
    pub dbm              : bool,
    pub contiguous       : bool,
    pub pxn              : bool,
    pub uxn              : bool,
    pub ignored_55       : bool,
    pub res_sw_use_58_56 : B3,  
    pub pbha             : B4,  
    pub ignored_63       : bool,
}
