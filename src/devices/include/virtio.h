#include "jerryTypes.h"
#include "libfdt.h"

/* Defined from virtio-v1.0-cs04 spec */

typedef volatile struct __attribute__((packed)) {
	u32 MagicValue;
	u32 Version;
	u32 DeviceID;
	u32 VendorID;
	u32 DeviceFeatures;
	u32 DeviceFeaturesSel;
	u32 _reserved0[2];
	u32 DriverFeatures;
	u32 DriverFeaturesSel;
	u32 _reserved1[2];
	u32 QueueSel;
	u32 QueueNumMax;
	u32 QueueNum;
	u32 _reserved2[2];
	u32 QueueReady;
	u32 _reserved3[2];
	u32 QueueNotify;
	u32 _reserved4[3];
	u32 InterruptStatus;
	u32 InterruptACK;
	u32 _reserved5[2];
	u32 Status;
	u32 _reserved6[3];
	u32 QueueDescLow;
	u32 QueueDescHigh;
	u32 _reserved7[2];
	u32 QueueAvailLow;
	u32 QueueAvailHigh;
	u32 _reserved8[2];
	u32 QueueUsedLow;
	u32 QueueUsedHigh;
	u32 _reserved9[21];
	u32 ConfigGeneration;
	u32 Config[0];
} virtioRegs;

#define VIRTIO_MAGIC   0x74726976
#define VIRTIO_VERSION 0x2
#define VIRTIO_DEV_NET 0x1
#define VIRTIO_DEV_BLK 0x2

#define VIRTIO_STATUS_ACKNOWLEDGE        (1)
#define VIRTIO_STATUS_DRIVER             (2)
#define VIRTIO_STATUS_FAILED             (128)
#define VIRTIO_STATUS_FEATURES_OK        (8)
#define VIRTIO_STATUS_DRIVER_OK          (4)
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET (64)

struct virtio_cap {
	char *name;
	u32 bit;
	bool support;
	char *help;
};

bool setupVirtIODevice(const void* deviceTreeAddress, s32 virtIODevNodeOffset);


