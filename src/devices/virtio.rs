use crate::{devices::*, read32};

pub enum VirtIODevice {
    Block(VirtIOBlk),
}

pub struct VirtIOBlk {
    size_bytes: u64
}

pub enum VirtIOError {
    WrongMagicValue,
    UnsupportedVersion,
    UnsupportedDeviceType,
    GetRegsFailed(FDTError),
    GetInterruptIDFailed(FDTError)
}

pub fn setup_virtio_device(virtio_node: FDTNode) -> Result<VirtIODevice, VirtIOError> { 
    // let virtio_regs: &'static VirtIORegs = match virtio_node.get_reg() {   
    //     Ok((virtio_regs_ptr, _virtio_regs_ptr_len)) => {
    //         unsafe {
    //             &*(virtio_regs_ptr as *const VirtIORegs)
    //         }
    //     }, 
    //     Err(e) => {
    //         return Err(VirtIOError::GetRegsFailed(e));
    //     }
    // };
    // 
    // let interrupt_id: u32 = match virtio_node.get_property::<u32>(b"interrupts\0") {
    //     Ok(node_regs) => {
    //         u32::from_be(node_regs[1]) + 32 // Idk why we have to add a constant 32...?
    //     },
    //     Err(e) => {
    //         return Err(VirtIOError::GetInterruptIDFailed(e))
    //     }
    // };
    // unsafe {
    //     let magic_value: u32 = read32(
    //         &virtio_regs.magic_value
    //     );
    //     if magic_value != VIRTIO_MAGIC {
    //         return Err(VirtIOError::WrongMagicValue);
    //     }
    // }

    Ok(VirtIODevice::Block(VirtIOBlk {size_bytes: 0}))
}

#[repr(C, align(4))]
pub struct VirtIORegs {
    magic_value: u32,
    version: u32,
    device_id: u32,
    vendor_id: u32,
    device_features: u32,
    device_features_sel: u32,
    _reserved0: [u32; 2],
    driver_features: u32,
    // Affects driver_features.
    // driver_features_sel=0 -> driver_features[31:0]
    // driver_features_sel=1 -> driver_features[63:32]
    driver_features_sel: u32,
    _reserved1: [u32; 2],
    queue_sel: u32,
    queue_num_max: u32,
    queue_num: u32,
    _reserved2: [u32; 2],
    queue_ready: u32,
    _reserved3: [u32; 2],
    queue_notify: u32,
    _reserved4: [u32; 3],
    interrupt_status: u32,
    interrupt_ack: u32,
    _reserved5: [u32; 2],
    status: u32,
    _reserved6: [u32; 3],
    queue_desc_low: u32,
    queue_desc_high: u32,
    _reserved7: [u32; 2],
    queue_avail_low: u32,
    queue_avail_high: u32,
    _reserved8: [u32; 2],
    queue_used_low: u32,
    queue_used_high: u32,
    _reserved9: [u32; 21],
    config_generation: u32,
    // config: [u32; 0] // Flexible Array Member
}


pub const VIRTIO_MAGIC:   u32 = 0x7472_6976;
pub const VIRTIO_VERSION: u32 = 0x2;

pub const VIRTIO_DEV_NET: u32 = 0x1;
pub const VIRTIO_DEV_BLK: u32 = 0x2;

// Status bit values
pub const VIRTIO_STATUS_ACKNOWLEDGE:        u32 = 1;
pub const VIRTIO_STATUS_DRIVER:             u32 = 2;
pub const VIRTIO_STATUS_FAILED:             u32 = 128;
pub const VIRTIO_STATUS_FEATURES_OK:        u32 = 8;
pub const VIRTIO_STATUS_DRIVER_OK:          u32 = 4;
pub const VIRTIO_STATUS_DEVICE_NEEDS_RESET: u32 = 64;