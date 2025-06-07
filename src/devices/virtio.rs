use crate::devices::*;

pub enum VirtIODevice {
    Block(VirtIOBlk),
}

pub struct VirtIOBlk {
    size_bytes: u64
}

pub enum VirtIOError {
    WrongMagicValue,
    UnsupportedVersion,
    UnsupportedDeviceType
}

// fn setup_virtio_device(jerry_meta_data: &JerryMetaData, virtio_dev_node_offset: i32) -> Result<VirtIODevice, VirtIOError> { 
//    
// }

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
