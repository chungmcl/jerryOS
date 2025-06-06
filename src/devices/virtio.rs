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
