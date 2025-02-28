#include "jerryTypes.h"

/* Defined from virtio-v1.0-cs04 spec */

/* Feature bits */
#define VIRTIO_BLK_F_SIZE_MAX   (1)  // Maximum size of any single segment is in size_max.
#define VIRTIO_BLK_F_SEG_MAX    (2)  // Maximum number of segments in a request is in seg_max.
#define VIRTIO_BLK_F_GEOMETRY   (4)  // Disk-style geometry specified in geometry.
#define VIRTIO_BLK_F_RO         (5)  // Device is read-only.
#define VIRTIO_BLK_F_BLK_SIZE   (6)  // Block size of disk is in blk_size.
#define VIRTIO_BLK_F_FLUSH      (9)  // Cache flush command support.
#define VIRTIO_BLK_F_TOPOLOGY   (10) // Device exports information on optimal I/O alignment.
#define VIRTIO_BLK_F_CONFIG_WCE (11) // Device can toggle its cache between writeback and writethrough modes.

/*
  This struct's fields must be little-endian.
  1. The device size can be read from capacity.
  2. If the VIRTIO_BLK_F_BLK_SIZE feature is negotiated, blk_size can be read to determine 
      the optimal sector size for the driver to use. This does not affect the units used 
      in the protocol (always 512 bytes), but awareness of the correct value can affect performance.
  3. If the VIRTIO_BLK_F_RO feature is set by the device, any write requests will fail.
  4. If the VIRTIO_BLK_F_TOPOLOGY feature is negotiated, the fields in the topology struct can be read
      to determine the physical block size and optimal I/O lengths for the driver to use. This also does not
      affect the units in the protocol, only performance.
  5. If the VIRTIO_BLK_F_CONFIG_WCE feature is negotiated, the cache mode can be read or set through
      the writeback field. 0 corresponds to a writethrough cache, 1 to a writeback cache4. The cache mode
      after reset can be either writeback or writethrough. The actual mode can be determined by reading
      writeback after feature negotiation.
*/
typedef struct _virtio_blk_config {
  u64 capacity;
  u32 size_max;
  u32 seg_max;
  struct virtio_blk_geometry {
    u16 cylinders;
    u8 heads;
    u8 sectors;
  } geometry;
  u32 blk_size;
  struct virtio_blk_topology {
    // # of logical blocks per physical block (log2)
    u8 physical_block_exp;
    // offset of first aligned logical block
    u8 alignment_offset;
    // suggested minimum I/O size in blocks
    u16 min_io_size;
    // optimal (suggested maximum) I/O size in blocks
    u32 opt_io_size;
  } topology;
  u8 writeback;
} virtio_blk_config;