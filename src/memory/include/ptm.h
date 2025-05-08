#include "jerryTypes.h"

/*
 * Notes for how to interpret what bits represent in TTEs via the A-Profile Reference:
 * 
 * 1. jerry will run on a Cortex-A710 processor.
 *    Cortex-A710 processor features (non-exhaustive, just some that affect TTE bits):
 *    Feature      Implemented?
 *    -----------  ------------
 *    FEAT_BBM     ❌            
 *    FEAT_BTI     ❌
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
 *    FEAT_NV2     ❌
 *    FEAT_S1PIE   ❌
 *      • -> Stage 1 Direct permissions are used
 *    FEAT_TCR2    ❌
 *      • -> TCR2_EL1, TCR2_EL2, and their associated trap controls are not implemented
 *      • note: Although the ARM ref. manual states "FEAT_TCR2 is mandatory from 
 *              Armv8.9" (ARM DDI 0487, Page A2-142), and jerry is running on a Cortex 
 *              A710 which is Armv9.0, it is important to note that ARM states that "Arm®v9.0-A 
 *              architecture extends the architecture defined in the Armv8-A architectures up to Arm®v8.5-A." 
 *              (https://developer.arm.com/documentation/101800/0201/The-Cortex-A710--core/Supported-standards-and-specifications) 
 *              -- in other words, Armv9.0 is not necessarily "greater than" Armv8.9; in reality it's Armv8.5
 *              with some extra features slapped on top. Therefore, it is expected that FEAT_TCR2
 *              is not implemented on the Cortex A710.
 *    FEAT_THE     ❌
 *      • -> Effective value of PnCH=0 & RES0
 *    FEAT_XNX     ❌
 * 
 * 2. jerry will use a 16KB translation granule.
 * 
 * 3. jerry will not use Secure state (no use of ARM Realms)
*/

/*
 * From ARM docs:
 * "Effective value A register control field, meaning a field in a register that controls some aspect of the behavior, can be
 * described as having an Effective value:
 * • In some cases, the description of a control a specifies that when control a is active it causes a
 *   register control field b to be treated as having a fixed value for all purposes other than direct
 *   reads, or direct reads and direct writes, of the register containing control field b. When control
 *   a is active that fixed value is described as the Effective value of register control field b. For
 *   example, when the value of HCR.DC is 1, the Effective value of HCR.VM is 1, regardless of
 *   its actual value.
 *   In other cases, in some contexts a register control field b is not implemented or is not
 *   accessible, but behavior of the PE is as if control field b was implemented and accessible, and
 *   had a particular value. In this case, that value is the Effective value of register control field b.
 *   Note:
 *     Where a register control field is introduced in a particular version of the architecture, and is not
 *     implemented in an earlier version of the architecture, typically it will have an Effective value in
 *     that earlier version of the architecture.
 * • Otherwise, the Effective value of a register control field is the value of that field."
*/

#define N_BITS(n) \
  ((1ULL << (n + 1)) - 1)

#define GET_BITS(from, msb, lsb) \
  (((from) >> lsb) & N_BITS(msb - lsb))
  
#define SET_BITS(from, to, msb, lsb) \
  ((to) = ((to) & ~(N_BITS(msb - lsb) << lsb)) | (((u64)(from) & N_BITS(msb - lsb)) << lsb))

/**************** STAGE 1 TABLE DESCRIPTOR DEFS ****************/

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

/*
 * Hardware managed Table descriptor Access flag is not enabled -> [10] is IGNORED
*/

// TCR_ELx.DS is 0 -> [9:8] is IGNORED

// [7:2] inherits ⬇️

// [1] inherits ⬇️

// [0] inherits ⬇️

/**************** END STAGE 1 TABLE DESCRIPTOR DEFS ****************/

/**************** STAGE 2 TABLE DESCRIPTOR DEFS ****************/

// [63] is RES0

// [62:59] is RES0 if Stage 2 Indirect permissions are disabled; else IGNORED

// [58:51] is IGNORED

// [50] is RESO0

// VTCR_EL2=0 -> [49:48] is RES0

/*
 * 16KB translation granule is used -> [47:14] is NLTA, [13:12] are RES0
*/
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

/*
 * Stage 1 direct permissions are used -> [7:6] is AP
*/
#define GET_AP(from)     (GET_BITS(from, 7, 6))
#define SET_AP(from, to) (SET_BITS(from, to, 7, 6))

// jerry will not use Secure state -> [5] is RES0

#define GET_ATTRINDX(from)     (GET_BITS(from, 4, 2))
#define SET_ATTRINDX(from, to) (SET_BITS(from, to, 4, 2))

// [0] inherits ⬇️

/*
 * FEAT_TCR2 is not implemented | FEAT_S1PIE is not implemented -> stage 1 direct permissions;
 * jerry will use EL1&0;
 * The translation regime supports two privilege levels -> [11] is Not global (nG)
*/
#define GET_NG(from)     (GET_BITS(from, 11, 11))
#define SET_NG(from, to) (SET_BITS(from, to, 11, 11))

/*
 * FEAT_LPA2 is not implemented -> TCR_ELx.DS is 0;
 * The 4KB or 16KB translation granule is used, and the Effective value of TCR_ELx.DS is 0 -> [49:48] is RES0
*/

// FEAT_BTI is not implemented -> [50] is RES0

// FEAT_THE is not implemented -> The Effective value of PnCH is 0 -> [52] is Contiguous (same as Stage 2)

/*
 * FEAT_TCR2 is not implemented | FEAT_S1PIE is not implemented -> stage 1 direct permissions;
 * The translation regime supports two privilege levels -> [53] is PXN
*/
#define GET_PXN(from)     (GET_BITS(from, 53, 53))
#define SET_PXN(from, to) (SET_BITS(from, to, 53, 53))

/*
 * FEAT_NV, FEAT_NV2 are not implemented -> HCR_EL2.{NV,NV1} cannot be {1,1};
 * jerry will use EL1&0 with HCR_EL2.{NV,NV1} = {0,0};
 * The translation regime supports two privilege levels ->
 * [54] is Unprivileged Execute-never (UXN)
*/
#define GET_UXN(from)     (GET_BITS(from, 54, 54))
#define SET_UXN(from, to) (SET_BITS(from, to, 54, 54))

// [55] is IGNORED

// [58:56] are reserved for software use

/*
 * jerry will not use Realm state;
 * Non-secure OR Secure state -> [63] is IGNORED
*/

/**************** END STAGE 1 PAGE/BLOCK DESCRIPTOR DEFS ****************/

/**************** STAGE 2 PAGE/BLOCK DESCRIPTOR DEFS ****************/
/*
 * In this table, OAB is the OA base that is appended to the IA 
 * supplied to the translation stage to produce the final OA 
 * supplied by the translation stage.
*/

// If value is 0, the descriptor is invalid
#define GET_VALID_BIT(from)     (GET_BITS(from, 0, 0))
#define SET_VALID_BIT(from, to) (GET_BITS(from, to, 0, 0))

/*
 * Descriptor Type = 0 -> Block descriptor, for lookup levels less than lookup level 3.
 * Descriptor type = 1 -> Page descriptor, for lookup level 3.
*/
#define GET_DESCRIPTOR_TYPE(from)     (GET_BITS(from, 1, 1))
#define SET_DESCRIPTOR_TYPE(from, to) (SET_BITS(from, to, 1, 1))

/*
 * jerry will have FWB=0; Effective value of HCR_EL2.FWB is 0 -> 
 * • stage 1 and stage 2 memory type and Cacheability attributes are combined
 * • [5:2] are MemAttr
*/
#define GET_MEMATTR(from)     (GET_BITS(from, 5, 2))
#define SET_MEMATTR(from, to) (SET_BITS(from, to, 5, 2))

/*
 * jerry uses direct permissions; Stage 2 Indirect permissions are disabled ->
 * • [7:6] are S2AP
*/
#define GET_S2AP(from)     (GET_BITS(from, 7, 6))
#define SET_S2AP(from, to) (SET_BITS(from, to, 7, 6))

// TCR_ELx.DS is 0; jerry will use a 16KB translation granule -> [9:8] are Shareability
#define GET_SHAREABILITY(from)     (GET_BITS(from, 9, 8))
#define SET_SHAREABILITY(from, to) (SET_BITS(from, to, 9, 8))

// Access flag (AF)
#define GET_AF(from)     (GET_BITS(from, 10, 10))
#define SET_AF(from, to) (SET_BITS(from, to, 10, 10))

// FEAT_XS is not implemented -> [11] is RES0

/*
 * jerry will use 16KB granule; 
 * Page descriptor & The 16KB translation granule is used -> [47:14] is OAB ([13:12] are RES0)
*/
#define GET_PAGE_OAB(from)     (GET_BITS(from, 47, 14))
#define SET_PAGE_OAB(from, to) (GET_BITS(from, to, 47, 14))

// FEAT_LPA, FEAT_BBM are not implemented & L2 Block descriptor -> [16:12] is RES0

/*
 * jerry will use 16KB granule; 
 * L2 Block descriptor & The 16KB translation granule is used -> [47:25] is OAB ([24:17] are RES0)
*/
#define GET_BLOCK_OAB(from)     (GET_BITS(from, 47, 25))
#define SET_BLOCK_OAB(from, to) (SET_BITS(from, to, 47, 25))

// FEAT_LPA2 is not implemented -> TCR_ELx.DS is 0 -> [49:48] are RES0

// FEAT_S1PIE is not implemented -> Stage 1 Indirect permissions not enabled -> [51] is the Dirty bit modifier (DBM)
#define GET_DBM(from)     (GET_BITS(from, 51, 51))
#define SET_DBM(from, to) (SET_BITS(from, to, 51, 51))


#define GET_CONTIGUOUS(from)     (GET_BITS(from, 52, 52))
#define SET_CONTIGUOUS(from, to) (SET_BITS(from, to, 52, 52))

// FEAT_XNX is not implemented -> [53] is RES0

// FEAT_XNX is not implemented -> [54] is Execute-never (XN)
#define GET_XN(from)     (GET_BITS(from, 54, 54))
#define SET_XN(from, to) (SET_BITS(from, to, 54, 54))

/*
 * jerry will not use Realm state;
 * Security state other than Realm state -> [55] is IGNORED
*/

// [57:56] are reserved for software use

/*
 * jerry will set VTCR_EL2.AssuredOnly to 0;
 * VTCR_EL2.AssuredOnly is 0 -> [58] is reserved for software use
*/

/*
 * FEAT_HPDS2: Translation Table Page-Based is implemented ->
 *  • [62:60] is PBHA[3:1] if PBHA bit is enabled 
 *    by the corresponding TCR_ELx.HWUnn control bit.
 *  • [62:60] is reserved for use by a System MMU if PBHA bit is NOT enabled 
 *    by the corresponding TCR_ELx.HWUnn control bit.
 * 
 *  • [59] is PBHA[0] if PBHA bit is enabled by the corresponding TCR_ELx.HWUnn control bit.
 * 
 *  • (See "Page Based Hardware attributes.")
*/
#define GET_PBHA(from)     (GET_BITS(from, 62, 59))
#define SET_PBHA(from, to) (SET_BITS(from, to, 62, 59))

/*
 * jerry will not use Realm state;
 * Non-secure OR Secure state -> [63] is RES0
*/

/**************** END STAGE 2 PAGE/BLOCK DESCRIPTOR DEFS ****************/
