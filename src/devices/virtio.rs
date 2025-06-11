use core::ptr::read;
use crate::{devices::*, read32, write32, read64, dsb, SBType};

pub fn setup_virtio_device(virtio_node: FDTNode) -> Result<VirtIODevice, VirtIOError> { 
    let virtio_regs: &'static mut VirtIORegs = match virtio_node.get_reg() {   
        Ok((virtio_regs_ptr, _virtio_regs_ptr_len)) => {
            unsafe {
                &mut *(virtio_regs_ptr as *mut VirtIORegs)
            }
        }, 
        Err(e) => {
            return Err(VirtIOError::GetRegsFailed(e));
        }
    };
    
    let interrupt_id: u32 = match virtio_node.get_property(b"interrupts\0") {
        Ok(node_regs) => {
            u32::from_be(node_regs[1]) + 32// Idk why we have to add a constant 32...?
        },
        Err(e) => {
            return Err(VirtIOError::GetInterruptIDFailed(e))
        }
    };

    unsafe {
        let magic_value: u32 = read32(
            &virtio_regs.magic_value
        );
        if magic_value != VIRTIO_MAGIC {
            return Err(VirtIOError::WrongMagicValue);
        }

        let version: u32 = read32(
            &virtio_regs.version
        );
        if version != VIRTIO_VERSION {
            return Err(VirtIOError::UnsupportedVersion);
        }

        // 1. Reset the device.
        write32(&mut virtio_regs.status, 0);
        dsb(SBType::Sy);

        // 2. Set the ACKNOWLEDGE status bit: the guest OS has notice the device.
        let mut prev_regs_status: u32 = read32(&virtio_regs.status);
        write32(&mut virtio_regs.status, prev_regs_status | VIRTIO_STATUS_ACKNOWLEDGE);
        dsb(SBType::Sy);

        // 3. Set the DRIVER status bit: the guest OS knows how to drive the device.
        prev_regs_status = read32(&virtio_regs.status);
        write32(&mut virtio_regs.status, prev_regs_status | VIRTIO_STATUS_DRIVER);
        dsb(SBType::Sy);
        
        let device_id: u32 = read32(&virtio_regs.device_id);
        match device_id {
            VIRTIO_DEV_BLK => {
                match setup_block_device(virtio_regs, interrupt_id) {
                    Ok(_) => {
                        return Ok(VirtIODevice::Block(VirtIOBlk { size_bytes: 0 }));
                    },
                    Err(_e) => {
                        return Err(VirtIOError::WrongMagicValue);
                    }
                }
            },
            // VIRTIO_DEV_NET => {
            // 
            // },
            _ => { 
                return Err(VirtIOError::UnsupportedDeviceType);
            }
        }
    }
}

fn setup_block_device(blk_dev_regs: &VirtIORegs, interrupt_id: u32) -> Result<u64, VirtIOError> {
    let mut before: u32;
    let mut after: u32;
    let mut capacity: u64;
    let blk_dev_config_ptr: *const VirtIOBlkConfig = blk_dev_regs.get_config::<VirtIOBlk>();
    return Ok(
        loop {
            unsafe {
                before = read32(&blk_dev_regs.config_generation);
                capacity = read64(&(*blk_dev_config_ptr).capacity) * read32(&(*blk_dev_config_ptr).blk_size) as u64;
                after = read32(&blk_dev_regs.config_generation);
            }
            if after == before { break capacity };
        }
    );
}

pub trait TrailingConfig {
    type ConfigStruct;
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
} impl VirtIORegs {
    pub fn get_config<T: TrailingConfig>(&self) -> *const T::ConfigStruct {
        let self_raw_ptr: *const u8 = (self as *const Self) as *const u8;
        let config_ptr: *const u8 = unsafe { self_raw_ptr.add(size_of::<VirtIORegs>()) };
        return config_ptr as *const T::ConfigStruct;
    }
}

pub enum VirtIODevice {
    Block(VirtIOBlk),
}

pub struct VirtIOBlk {
    size_bytes: u64
} impl TrailingConfig for VirtIOBlk {
    type ConfigStruct = VirtIOBlkConfig;
}

#[repr(C)] 
#[derive(Debug, Copy, Clone)] 
pub struct VirtIOBlkConfig {
    pub capacity: u64,
    pub size_max: u32,
    pub seg_max: u32,
    pub geometry: VirtIOBlkGeometry,
    pub blk_size: u32,
    pub topology: VirtIOBlkTopology,
    pub writeback: u8
} #[repr(C)] #[derive(Debug, Copy, Clone)] pub struct VirtIOBlkGeometry {
    pub cylinders: u16,
    pub heads: u8,
    pub sectors: u8,
} #[repr(C)] #[derive(Debug, Copy, Clone)] pub struct VirtIOBlkTopology {
    pub physical_block_exp: u8,
    pub alignment_offset: u8,
    pub min_io_size: u16,
    pub opt_io_size: u32,
} 

pub enum VirtIOError {
    WrongMagicValue,
    UnsupportedVersion,
    UnsupportedDeviceType,
    GetRegsFailed(FDTError),
    GetInterruptIDFailed(FDTError)
}

const VIRTIO_MAGIC:                     u32 = 0x7472_6976;
const VIRTIO_VERSION:                   u32 = 0x2;

const VIRTIO_DEV_NET:                   u32 = 0x1;
const VIRTIO_DEV_BLK:                   u32 = 0x2;
// ...

// Status bit values
const VIRTIO_STATUS_ACKNOWLEDGE:        u32 = 1;
const VIRTIO_STATUS_DRIVER:             u32 = 2;
const VIRTIO_STATUS_FAILED:             u32 = 128;
const VIRTIO_STATUS_FEATURES_OK:        u32 = 8;
const VIRTIO_STATUS_DRIVER_OK:          u32 = 4;
const VIRTIO_STATUS_DEVICE_NEEDS_RESET: u32 = 64;