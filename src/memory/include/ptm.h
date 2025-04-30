#include "jerryTypes.h"

/*
 * For the Cortex-A710 processor jerry is designed to run on:
 * • FEAT_LPA is not implemented on Cortex-A710 -> VCTR_EL2.DS is 0 -> 48-bit NLTA
 * • FEAT_HAFDBS is implemented but not FEAT_HAFT on Cortex-A710
 * • FEAT_THE is not implemented -> bit 52 is the Contiguous bit
 * • FEAT_S1PIE is not implemented -> bit 51 is the Dirty bit modifier (DBM)
 * • FEAT_BBM is not implemented -> bit 16 is RES0 in a block descriptor
 * • FEAT_HPDS2 is implemented
 * In addition, design decisions that we are making:
 * • Granule Size = 16KB
 *    • -> For the block descriptors:
 *      • OAB is [47:36] if the Effective value of TCR_ELx.DS is 1, 
 *        and the descriptor is a level 1 Block descriptor. 
 *        Descriptor bits [35:17] are RES0.
 *      • OAB is [47:25] if the descriptor is a level 2 Block descriptor. 
 *        Descriptor bits [24:17] are RES0.
 *    • -> For the page descriptors:
 *      • OAB is [47:14]. 
 *        Descriptor bits [13:12] are RES0.
 *    • "OAB is the OA base that is appended to the IA supplied 
 *       to the translation stage to produce the final OA supplied
 *       by the translation stage."
 * • OA (output address) Size = 48-bits (sort of required due to absence of FEAT_LPA)
 */

#define N_BITS(n) ((1ULL << (n + 1)) - 1)
#define GET_BITS(from, msb, lsb) \
  (((from) >> lsb) & N_BITS(msb - lsb))
#define SET_BITS(from, to, msb, lsb) \
  ((to) = ((to) & ~(N_BITS(msb - lsb) << lsb)) | (((u64)(from) & N_BITS(msb - lsb)) << lsb))

#define GET_VALID_BIT(from)     (GET_BITS(from, 0, 0))
#define SET_VALID_BIT(from, to) (GET_BITS(from, to, 0, 0))

#define GET_TABLE_DESCRIPTOR(from)     (GET_BITS(from, 1, 1))
#define SET_TABLE_DESCRIPTOR(from, to) (SET_BITS(from, 1, 1))

// Same for all stages
#define GET_ACCESS_FLAG(from)     (GET_BITS(from, 10, 10))
#define SET_ACCESS_FLAG(from, to) (SET_BITS(from, to, 10, 10))

#define GET_NEXT_LEVEL_TABLE_ADDRESS(from)     (GET_BITS(from, 47, 14))
#define SET_NEXT_LEVEL_TABLE_ADDRESS(from, to) (SET_BITS(from, to, 47, 14))

/*
 *  ________________________________________________________
 *  | Stage 1 and below (unless overridden by lower stage) |
 *  --------------------------------------------------------
 * 
 * In this table, OAB is the OA base that is appended to the IA 
 * supplied to the translation stage to produce the final OA 
 * supplied by the translation stage.
 */
#define GET_ATTRINDX(from)     (GET_BITS(from, 4, 2))
#define SET_ATTRINDX(from, to) (SET_BITS(from, to, 4, 2))

/* 
 * jerry will not use Realm state;
 * The access is from Non-secure state -> [5] is RES0
 */

/*
 * TODO: [7:6]
 */

// TODO: [11]

/*
 * FEAT_LPA2 is not implemented -> TCR_ELx.DS is 0;
 * The 4KB or 16KB translation granule is used, and the Effective value of TCR_ELx.DS is 0 -> [49:48] is RES0
 */

// FEAT_BTI is not implemented -> [50] is RES0

// FEAT_THE is not implemented -> The Effective value of PnCH is 0 -> [52] is Contiguous (same as Stage 2)

// TODO: [53]

// TODO: [54]

// [55] is IGNORED

// [58:56] are reserved for software use

/*
 * jerry will not use Realm state;
 * Non-secure OR Secure state -> [63] is IGNORED
 */

/***********************************END STAGE 1********************************************* */

/*
 *  ________________________________________________________
 *  | Stage 2 and below (unless overridden by lower stage) |
 *  --------------------------------------------------------
 * 
 * In this table, OAB is the OA base that is appended to the IA 
 * supplied to the translation stage to produce the final OA 
 * supplied by the translation stage.
 */
// If value is 0, the descriptor is invalid
#define GET_VALID_BIT(from)     (GET_BITS(from, 0, 0))
#define SET_VALID_BIT(from, to) (GET_BITS(from, to, 0, 0))

/*
 * Block descriptor 0, for lookup levels less than lookup level 3.
 * Page descriptor 1, for lookup level 3.
 */
#define GET_DESCRIPTOR_TYPE(from)     (GET_BITS(from, 1, 1))
#define SET_DESCRIPTOR_TYPE(from, to) (SET_BITS(from, to, 1, 1))

/*
 * jerry will have FWB=0; Effective value of HCR_EL2.FWB is 0 -> 
 * • stage 1 and stage 2 memory type and Cacheability attributes are combined
 * • [5:2] are MemAttr
 */
#define GET_MEMATTR(from) (GET_BITS(from, 5, 2))
#define SET_MEMATTR(from, to) (SET_BITS(from, to, 5, 2))

/*
 * jerry uses direct permissions; Stage 2 Indirect permissions are disabled ->
 * • [7:6] are S2AP
 */
#define GET_S2AP(from) (GET_BITS(from, 7, 6))
#define SET_S2AP(from, to) (SET_BITS(from, to, 7, 6))

/*
 * jerry uses 16KB granule; FEAT_LPA2 is not implemented -> TCR_ELx.DS is 0; 
 * The 4KB or 16KB translation granule is used, and the Effective value of TCR_ELx.DS is 0 ->
 * • [9:8] are Shareability
 */
#define GET_SHAREABILITY(from) (GET_BITS(from, 9, 8))
#define SET_SHAREABILITY(from, to) (SET_BITS(from, to, 9, 8))

/*
 * jerry will have Hardware managed Table descriptor Access flag enabled;
 * Hardware managed Table descriptor Access flag is enabled -> [10] is Access flag (AF)
*/
#define GET_AF(from)     (GET_BITS(from, 10, 10))
#define SET_AF(from, to) (SET_BITS(from, to, 10, 10))

// FEAT_XS is not implemented -> [11] is RES0

/*
 * jerry will use 16KB granule; 
 * Page descriptor & The 16KB translation granule is used -> [47:14] is OAB ([13:12] are RES0)
 */
#define GET_PAGE_OAB(from) (GET_BITS(from, 47, 14))
#define SET_PAGE_OAB(from, to) (GET_BITS(from, to, 47, 14))

// FEAT_LPA, FEAT_BBM are not implemented & L2 Block descriptor -> [16:12] is RES0

/*
 * jerry will use 16KB granule; 
 * L2 Block descriptor & The 16KB translation granule is used -> [47:25] is OAB ([24:17] are RES0)
 */
#define GET_BLOCK_OAB(from) (GET_BITS(from, 47, 25))
#define SET_BLOCK_OAB(from, to) (SET_BITS(from, to, 47, 25))

// FEAT_LPA2 is not implemented -> TCR_ELx.DS is 0 -> [49:48] are RES0

// FEAT_S1PIE is not implemented -> Stage 1 Indirect permissions not enabled -> [51] is the Dirty bit modifier (DBM)
#define GET_DBM(from) (GET_BITS(from, 51, 51))
#define SET_DBM(from, to) (SET_BITS(from, to, 51, 51))


#define GET_CONTIGUOUS(from) (GET_BITS(from, 52, 52))
#define SET_CONTIGUOUS(from, to) (SET_BITS(from, to, 52, 52))

// FEAT_XNX is not implemented -> [53] is RES0

// FEAT_XNX is not implemented -> [54] is Execute-never (XN)
#define GET_XN(from) (GET_BITS(from, 54, 54))
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
 * FEAT_HPDS2, Translation Table Page-Based is implemented ->
 *  • [62:60] is PBHA[3:1] if PBHA bit is enabled 
 *    by the corresponding TCR_ELx.HWUnn control bit.
 *  • [62:60] is reserved for use by a System MMU if PBHA bit is NOT enabled 
 *    by the corresponding TCR_ELx.HWUnn control bit.
 * 
 *  • [59] is PBHA[0] if PBHA bit is enabled by the corresponding TCR_ELx.HWUnn control bit.
 * 
 *  • (See "Page Based Hardware attributes.")
 */
#define GET_PBHA(from) (GET_BITS(from, 62, 59))
#define SET_PBHA(from, to) (SET_BITS(from, to, 62, 59))

/*
 * jerry will not use Realm state;
 * Non-secure OR Secure state -> [63] is RES0
 */