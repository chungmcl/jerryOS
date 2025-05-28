// Kevin the Page Table Manager

#include "jerryOS.h"

bool setupPTM(const hardwareInfo*const hwInfo);

typedef union {
  u64 raw;
  struct __attribute__((packed)) {
    u8 T0SZ       : 6;
    u8 res0_6     : 1;
    u8 EPD0       : 1;
    u8 IRGN0      : 2;
    u8 ORGN0      : 2;
    u8 SH0        : 2;
    u8 TG0        : 2;
    u8 T1SZ       : 6;
    u8 A1         : 1;
    u8 EPD1       : 1;
    u8 IRGN1      : 2;
    u8 ORGN1      : 2;
    u8 SH1        : 2;
    u8 TG1        : 2;
    u8 IPS        : 3;
    u8 res0_35    : 1;
    u8 AS         : 1;
    u8 TBI0       : 1;
    u8 TBI1       : 1;
    u8 HA         : 1; // res0 if FEAT_HAFDBS is not implemented
    u8 HD         : 1; // res0 if FEAT_HAFDBS is not implemented
    u8 HPD0       : 1; // res0 if FEAT_HPDS is not implemented
    u8 HPD1       : 1; // res0 if FEAT_HPDS is not implemented
    u8 HWU059     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 HWU060     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 HWU061     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 HWU062     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 HWU159     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 HWU160     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 HWU161     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 HWU162     : 1; // res0 if FEAT_HPDS2 is not implemented
    u8 TBID0      : 1; // res0 if FEAT_PAuth not implemented
    u8 TBID1      : 1; // res0 if FEAT_PAuth not implemented
    u8 NFD0       : 1; // res0 if FEAT_SVE and FEAT_TME are not implemented
    u8 NFD1       : 1; // res0 if FEAT_SVE and FEAT_TME are not implemented
    u8 E0PD0      : 1; // res0 if FEAT_E0PD is not implemented
    u8 E0PD1      : 1; // res0 if FEAT_E0PD is not implemented
    u8 TCMA0      : 1; // res0 if FEAT_MTE2 is not implemented
    u8 TCMA1      : 1; // res0 if FEAT_MTE2 is not implemented
    u8 DS         : 1; // res0, Effective Value of 0 if FEAT_LPA2 is not implemented or (FEAT_D128 is implemented and TCR2_EL1.D128 == 0)
    u8 MTX0       : 1; // res0 if FEAT_MTE_NO_ADDRESS_TAGS and FEAT_MTE_CANONICAL_TAGS are not implemented
    u8 MTX1       : 1; // res0 if FEAT_MTE_NO_ADDRESS_TAGS and FEAT_MTE_CANONICAL_TAGS are not implemented
    u8 res0_63_62 : 2;
  };
} regTCR_EL1;
