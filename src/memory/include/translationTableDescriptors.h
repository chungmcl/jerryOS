#include "jerryOS.h"

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

#define N_BITS(n) \
  ((1ULL << (n + 1)) - 1)

#define GET_BITS(from, msb, lsb) \
  (((from) >> lsb) & N_BITS(msb - lsb))
  
#define SET_BITS(from, to, msb, lsb) \
  ((to) = ((to) & ~(N_BITS(msb - lsb) << lsb)) | (((u64)(from) & N_BITS(msb - lsb)) << lsb))

/**************** STAGE 1 TABLE DESCRIPTOR DEFS ****************/
typedef union {
  u64 raw;
  struct __attribute__((packed)) {
    u8  validBit        :  1;
    u8  tableDescriptor :  1;
    u8  ignored_7_2     :  6;
    u8  ignored_9_8     :  2;
    u8  ignored_10      :  1;
    u8  ignored_11      :  1;
    u8  res0_13_12      :  2;
    u64 nlta            : 34;
    u8  res0_49_48      :  2;
    u8  res0_50         :  1;
    u8  ignored_51      :  1;
    u8  ignored_52      :  1;
    u8  ignored_58_53   :  6;
    u8  pxnTable        :  1;
    u8  uxnTable        :  1;
    u8  apTable         :  2;
    u8  res0_63         :  1;
  };
} tableDescriptorS1;

// jerry will not use Secure state -> [63] is RES0

/*
 * Hierarchical permissions are enabled -> [62:61] is APTable
*/
#define GET_APTABLE(from)     (GET_BITS(from 62, 61))
#define SET_APTABLE(from, to) (SET_BITS(from, to, 62, 61))

/*
 * Hierarchical permissions are enabled and the translation 
 * regime supports two privilege levels -> [60] is UXNTable
*/
#define GET_UXNTABLE(from)     (GET_BITS(from, 60, 60))
#define SET_UXNTABLE(from, to) (SET_BITS(from, to, 60, 60))

/*
 * Hierarchical permissions are enabled and the translation 
 * regime supports two privilege levels -> [59] is PXNTable
*/
#define GET_PXNTABLE(from)     (GET_BITS(from, 59, 59))
#define SET_PXNTABLE(from, to) (SET_BITS(from, to, 59, 59))

// [58:53] is IGNORED

// PnCH=0 -> [52] is IGNORED

// [51] is IGNORED

// [50] is RES0

// TCR_ELx=0 -> [49:48] is RES0

// [47:12] inherits ⬇️

// [11] is IGNORED

// Hardware managed Table descriptor Access flag is not enabled -> [10] is IGNORED

// TCR_ELx.DS is 0 -> [9:8] is IGNORED

// [7:2] are IGNORED

// [1] inherits ⬇️

// [0] inherits ⬇️

/**************** END STAGE 1 TABLE DESCRIPTOR DEFS ****************/

/**************** STAGE 2 TABLE DESCRIPTOR DEFS ****************/
typedef union {
  u64 raw;
  struct __attribute__((packed)) {
    u8  validBit           :  1;
    u8  tableDescriptor    :  1;
    u8  ignored_7_2        :  6;
    u8  ignored_9_8        :  2;
    u8  ignored_10         :  1;
    u8  ignored_11         :  1;
    u8  res0_13_12         :  2;
    u64 nlta               : 34;
    u8  res0_49_48         :  2;
    u8  res0_50            :  1;
    u8  ignored_58_51      :  8;
    u8  res0_ignored_62_59 :  4;
    u8  res0_63            :  1;
  };
} tableDescriptorS2;

// [63] is RES0

// [62:59] is RES0 if Stage 2 Indirect permissions are disabled; else IGNORED

// [58:51] is IGNORED

// [50] is RES0

// VTCR_EL2=0 -> [49:48] is RES0


// 16KB translation granule is used -> [47:14] is Next-level Table Address (NLTA), [13:12] are RES0
#define GET_NLTA(from)     (GET_BITS(from, 47, 14))
#define SET_NLTA(from, to) (SET_BITS(from, to, 47, 14))

// [11] is IGNORED

// Hardware managed Table descriptor Access flag is not enabled -> [10] is IGNORED

// jerry will use a 16KB translation granule & VTCR_EL2.DS is 0 -> [9:8] is IGNORED

// [7:2] are IGNORED

#define GET_TABLE_DESCRIPTOR(from)     (GET_BITS(from, 1, 1))
#define SET_TABLE_DESCRIPTOR(from, to) (SET_BITS(from, 1, 1))

// [0] inherits ⬇️

/**************** END STAGE 2 TABLE DESCRIPTOR DEFS ****************/

/**************** STAGE 1 PAGE/BLOCK DESCRIPTOR DEFS ****************/
/*
 * In this table, OAB is the OA base that is appended to the IA 
 * supplied to the translation stage to produce the final OA 
 * supplied by the translation stage.
*/

typedef union { 
  u64 raw;
  struct __attribute__((packed)) {
    u8  validBit       :  1;
    u8  descriptorType :  1;
    u8  attrIndx       :  3;
    u8  res0_5         :  1;
    u8  ap             :  2;
    u8  shareability   :  2;
    u8  af             :  1;
    u8  nG             :  1;
    u8  res0_13_12     :  2;
    u64 oab            : 34;
    u8  res0_49_48     :  2;
    u8  gp             :  1;
    u8  dbm            :  1;
    u8  contiguous     :  1;
    u8  pxn            :  1;
    u8  uxn            :  1;
    u8  ignored_55     :  1;
    u8  resSwUse_58_69 :  3;
    u8  pbha           :  4;
    u8  ignored_63     :  1;
  };
} pageDescriptorS1;
typedef union {
  u64 raw;
  struct __attribute__((packed)) {
    u8  validBit       :  1;
    u8  descriptorType :  1;
    u8  attrIndx       :  3;
    u8  res0_5         :  1;
    u8  ap             :  2;
    u8  shareability   :  2;
    u8  af             :  1;
    u8  nG             :  1;
    u8  res0_15_12     :  4;
    u8  nT             :  1;
    u8  res0_24_17     :  8;
    u64 oab            : 23;
    u8  res0_49_48     :  2;
    u8  gp             :  1;
    u8  dbm            :  1;
    u8  contiguous     :  1;
    u8  pxn            :  1;
    u8  uxn            :  1;
    u8  ignored_55     :  1;
    u8  resSwUse_58_56 :  3;
    u8  pbha           :  4;
    u8  ignored_63     :  1;
  };
} blockDescriptorS1;

// jerry will not use Realms -> [63] is IGNORED

// [62:59] inherits ⬇️ 

// [58:56] is Reserved for software use

// [55] is IGNORED

/*
 * HCR_EL2.{NV, NV1}={0, 0} & two privilege levels ->
 * [54] is Unprivileged Execute-never (UXN)
*/
#define GET_UXN(from)     (GET_BITS(from, 54, 54))
#define SET_UXN(from, to) (SET_BITS(from, to, 54, 54))

/*
 * HCR_EL2.{NV, NV1}={0, 0} & two privilege levels ->
 * -> [53] is PXN 
*/
#define GET_PXN(from)     (GET_BITS(from, 53, 53))
#define SET_PXN(from, to) (SET_BITS(from, to, 53, 53))

// [52] inherits ⬇️

// [51] inherits ⬇️

// FEAT_BTI is implemented -> [50] is Guarded Page (GP)
#define GET_GP(from)     (GET_BITS(from, 50, 50))
#define SET_GP(from, to) (SET_BITS(from, to, 50, 50))

// TCR_ELx.DS is 0 & 16KB translation granule is used -> [49:48] is RES0

// [47:12] inherits ⬇️

// The translation regime supports two privilege levels -> [11] is Not global (nG)
#define GET_NG(from)     (GET_BITS(from, 11, 11))
#define SET_NG(from, to) (SET_BITS(from, to, 11, 11))

// [10] inherits ⬇️

// [9:8] inherits ⬇️

// Stage 1 direct permissions are used -> [7:6] is AP
#define GET_AP(from)     (GET_BITS(from, 7, 6))
#define SET_AP(from, to) (SET_BITS(from, to, 7, 6))

// jerry will not use Secure state -> [5] is RES0

#define GET_ATTRINDX(from)     (GET_BITS(from, 4, 2))
#define SET_ATTRINDX(from, to) (SET_BITS(from, to, 4, 2))

// [1] inherits ⬇️

// [0] inherits ⬇️

/**************** END STAGE 1 PAGE/BLOCK DESCRIPTOR DEFS ****************/

/**************** STAGE 2 PAGE/BLOCK DESCRIPTOR DEFS ****************/
/*
 * In this table, OAB is the OA base that is appended to the IA 
 * supplied to the translation stage to produce the final OA 
 * supplied by the translation stage.
*/
typedef union {
  u64 raw;
  struct __attribute__((packed)) {
    u8  validBit       :  1;
    u8  descriptorType :  1;
    u8  memAttr        :  4;
    u8  s2ap           :  2;
    u8  shareability   :  2;
    u8  af             :  1;
    u8  res0_11        :  1; 
    u8  res0_13_12     :  2;
    u64 oab            : 34;
    u8  res0_49_48     :  2;
    u8  res0_50        :  1;
    u8  dbm            :  1;
    u8  contiguous     :  1;
    u8  xn             :  2;
    u8  ignored_55     :  1;
    u8  resSwUse_57_56 :  2;
    u8  resSwUse_58    :  1; 
    u8  pbha           :  4;
    u8  ignored_63     :  1;
  };
} pageDescriptorS2;
typedef union {
  u64 raw;
  struct __attribute__((packed)) {
    u8  validBit       :  1;
    u8  descriptorType :  1;
    u8  memAttr        :  4;
    u8  s2ap           :  2;
    u8  shareability   :  2;
    u8  af             :  1;
    u8  res0_11        :  1;
    u8  res0_15_12     :  4;
    u8  nT             :  1;
    u8  res0_24_17     :  8;
    u64 oab            : 23;
    u8  res0_49_48     :  2;
    u8  res0_50        :  1;
    u8  dbm            :  1;
    u8  contiguous     :  1;
    u8  xn             :  2;
    u8  ignored_55     :  1;
    u8  resSwUse_57_56 :  2;
    u8  resSwUse_58    :  1; 
    u8  pbha           :  4;
    u8  ignored_63     :  1;
  };
} blockDescriptorS2;

// jerry will not use Realms; Non-secure OR Secure state -> [63] is RES0

/*
 * FEAT_HPDS2 is implemented & FEAT_AIE is not implemented & FEAT_S{1,2}POE are not implemented ->
 *  • [62:60] is:
 *    • PBHA[3:1], if PBHA bit is enabled by the corresponding TCR_ELx.HWUnn control bit.
 *    • reserved for use by a System MMU, if Stage 2 descriptor & PBHA bit is (sic, 
 *      probably "is NOT?") enabled by the corresponding TCR_ELx.HWUnn control bit.
 *    • IGNORED, if Stage 1 descriptor & [62:60] is IGNORED if PBHA bit is not enabled 
 *      by the corresponding TCR_ELx.HWUnn control bit.
 * 
 *  • [59] is PBHA[0] if PBHA bit is enabled by the corresponding TCR_ELx.HWUnn control bit, else IGNORED.
*/
#define GET_PBHA(from)     (GET_BITS(from, 62, 59))
#define SET_PBHA(from, to) (SET_BITS(from, to, 62, 59))

// VTCR_EL2.AssuredOnly is 0 -> [58] is reserved for software use

// [57:56] are reserved for software use

// jerry will not use Realms; Security state other than Realm state -> [55] is IGNORED

// FEAT_XNX is implemented & Stage 2 Indirect permissions disabled -> [54:53] is XN
#define GET_XN(from)     (GET_BITS(from, 54, 53))
#define SET_XN(from, to) (SET_BITS(from, to, 54, 53))

/*
 * • For Stage 1, PnCH=0 -> [52] is Contiguous 
 * • For Stage 2, [52] is Contiguous
*/
#define GET_CONTIGUOUS(from)     (GET_BITS(from, 52, 52))
#define SET_CONTIGUOUS(from, to) (SET_BITS(from, to, 52, 52))

// Stage {1,2} Indirect permissions are disabled -> [51] is the Dirty bit modifier (DBM)
#define GET_DBM(from)     (GET_BITS(from, 51, 51))
#define SET_DBM(from, to) (SET_BITS(from, to, 51, 51))

// [50] is RES0

// jerry will use 16KB granule & TCR_ELx.DS is 0 -> [49:48] are RES0

/*
 * jerry will use 16KB granule ->
 * • if Page descriptor -> [47:14] is OAB ([13:12] are RES0)
 * • if Block descriptor -> [47:25] is OAB ([24:17] are RES0)
*/
#define GET_PAGE_OAB(from)      (GET_BITS(from, 47, 14))
#define SET_PAGE_OAB(from, to)  (GET_BITS(from, to, 47, 14))
#define GET_BLOCK_OAB(from)     (GET_BITS(from, 47, 25))
#define SET_BLOCK_OAB(from, to) (SET_BITS(from, to, 47, 25))
// FEAT_BBM is implemented -> Block[16] = nT (See Block translation entry)
#define GET_BLOCK_NT(from)      (GET_BITS(from, 16, 16))
#define SET_BLOCK_NT(from, to)  (SET_BITS(from, to, 16, 16))
// FEAT_LPA is not implemented -> Block[15:12] is RES0

// FEAT_XS is not implemented -> [11] is RES0

// Access flag (AF)
#define GET_AF(from)     (GET_BITS(from, 10, 10))
#define SET_AF(from, to) (SET_BITS(from, to, 10, 10))

// TCR_ELx.DS is 0 & jerry will use a 16KB translation granule -> [9:8] are Shareability
#define GET_SHAREABILITY(from)     (GET_BITS(from, 9, 8))
#define SET_SHAREABILITY(from, to) (SET_BITS(from, to, 9, 8))

// Stage 2 direct permissions are used -> [7:6] are S2AP
#define GET_S2AP(from)     (GET_BITS(from, 7, 6))
#define SET_S2AP(from, to) (SET_BITS(from, to, 7, 6))

/*
 * • [5] is RES0 if HCR_EL2.FWB is set to 1
 * • [5] is MemAttr[3] if HCR_EL2.FWB is set to 0
 * • (See "Stage 2 memory type and Cacheability attributes when FWB is disabled/enabled")
*/
#define GET_MEMATTR(from)     (GET_BITS(from, 5, 2))
#define SET_MEMATTR(from, to) (SET_BITS(from, to, 5, 2))

/*
 * Descriptor Type = 0 -> Block descriptor, for lookup levels less than lookup level 3.
 * Descriptor type = 1 -> Page descriptor, for lookup level 3.
*/
#define GET_DESCRIPTOR_TYPE(from)     (GET_BITS(from, 1, 1))
#define SET_DESCRIPTOR_TYPE(from, to) (SET_BITS(from, to, 1, 1))

// If value is 0, the descriptor is invalid
#define GET_VALID_BIT(from)     (GET_BITS(from, 0, 0))
#define SET_VALID_BIT(from, to) (GET_BITS(from, to, 0, 0))

/**************** END STAGE 2 PAGE/BLOCK DESCRIPTOR DEFS ****************/
