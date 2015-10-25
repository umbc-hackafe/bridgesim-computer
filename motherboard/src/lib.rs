extern crate libc;

use libc::c_void;

// Rusty Section

/// Represents how memory is mapped for a `Device`
///
/// Different devices present different types of memory; devices intended just to be used
/// as RAM will be mapped to memory differently than devices which provide I/O.
#[repr(C)]
#[derive(Copy, Clone)]
pub enum MappedMemoryType {
    /// No memory will be mapped for the device.
    None = 0,
    /// Memory will be mapped for the device, and it will be mapped in the RAM section of
    /// system memory.
    Ram = 1,
    /// Memory will be mapped for the device, and it will be mapped in the I/O Devices
    /// section of system memory.
    IODevice = 2,
}

/// Represents a device handled by a motherboard
///
/// All of the device functions supplied should return an error code if they fail. However
/// this should only be used for signaling the motherboard of invalid *simulator*
/// behavior. Invalid behavior of a *simulated* part should be handled through in-sim
/// methods like interrupts.
///
/// # Safety
///
/// The device operation functions provided in a `Device` must be made thread-safe by the
/// implementor. The motherboard and other devices may call them freely in a
/// multi-threaded environment.
///
/// The `load_*` functions take a pointer to the memory to place the loaded value in. To
/// comply with Rust's ownership semantics, the actual implementation must not hold on to
/// this pointer past the lifetime of the function.
#[repr(C)]
#[derive(Copy, Clone)]
#[allow(raw_pointer_derive)]
pub struct Device {
    /// Pointer to the actual underlying device being operated on.
    pub device: *mut c_void,

    /// Indicated whether memory mapping should be done for this device, and what type of
    /// mapping should be used.
    pub export_memory: MappedMemoryType,
    /// Indicates the size of exported memory
    pub export_memory_size: u32,
    // Device, Address, Destination/Source
    /// Function to load a byte from the device's memory.
    pub load_byte: Option<extern fn(*mut c_void, u32, *mut u8) -> i32>,
    /// Function to load a word from the device's memory.
    pub load_word: Option<extern fn(*mut c_void, u32, *mut u16) -> i32>,
    /// Function to load a dword from the device's memory.
    pub load_dword: Option<extern fn(*mut c_void, u32, *mut u32) -> i32>,
    /// Function to load a qword from the device's memory.
    pub load_qword: Option<extern fn(*mut c_void, u32, *mut u64) -> i64>,
    /// Function to save a byte to the device's memory.
    pub write_byte: Option<extern fn(*mut c_void, u32, u8) -> i32>,
    /// Function to save a word to the device's memory.
    pub write_word: Option<extern fn(*mut c_void, u32, u16) -> i32>,
    /// Function to save a dword to the device's memory.
    pub write_dword: Option<extern fn(*mut c_void, u32, u32) -> i32>,
    /// Function to save a qword to the device's memory.
    pub write_qword: Option<extern fn(*mut c_void, u32, u64) -> i32>,

    // Device
    /// Function to use to initialize the device before booting.
    pub init: Option<extern fn(*mut c_void) -> i32>,

    // Device
    /// Optional function to use to boot the device.
    pub boot: Option<extern fn(*mut c_void) -> i32>,

    // Device, Interrupt Code
    /// Optional function to use to send an interrupt code to the device.
    pub interrupt: Option<extern fn(*mut c_void, u32) -> i32>,

    // Device, Motherboard, Motherboard Functions
    /// Registers the motherboard's functions with this device
    pub register_motherboard: Option<extern fn(
        *mut c_void, *mut Motherboard, *mut MotherboardFunctions) -> i32>,
}

/// Struct for passing the motherboard's function collection to modules.
#[repr(C)]
#[derive(Copy, Clone)]
#[allow(raw_pointer_derive)]
pub struct MotherboardFunctions {
    // Motherboard, Address, Destination.
    /// Callback for a device to read a byte of memory.
    pub read_byte: Option<extern fn(*mut Motherboard, u64, *mut u8) -> i32>,
    /// Callback for a device to read a word of memory.
    pub read_word: Option<extern fn(*mut Motherboard, u64, *mut u16) -> i32>,
    /// Callback for a device to read a dword of memory.
    pub read_dword: Option<extern fn(*mut Motherboard, u64, *mut u32) -> i32>,
    /// Callback for a device to read a qword of memory.
    pub read_qword: Option<extern fn(*mut Motherboard, u64, *mut u64) -> i32>,

    // Motherboard, Address, Source.
    /// Callback for a device to write a byte of memory.
    pub write_byte: Option<extern fn(*mut Motherboard, u64, u8) -> i32>,
    /// Callback for a device to write a word of memory.
    pub write_word: Option<extern fn(*mut Motherboard, u64, u16) -> i32>,
    /// Callback for a device to write a dword of memory.
    pub write_dword: Option<extern fn(*mut Motherboard, u64, u32) -> i32>,
    /// Callback for a device to write a qword of memory.
    pub write_qword: Option<extern fn(*mut Motherboard, u64, u64) -> i32>,

    // Motherboard, Target Device, Interrupt Code.
    /// Callback for a device to send an interrupt to another device.
    pub send_interrupt: Option<extern fn(*mut Motherboard, u32, u32) -> i32>,
}

/// Represents a motherboard in the bridgesim computer.
///
/// The motherboard has a fixed set of spaces for `Device` objects, which are anything
/// which can do or hold something. The motherboard is the core of the bridgesim computer
/// and is responsible for booting attached devices and memory access control.
pub struct Motherboard {
    devices: Vec<Device>,
}

impl Motherboard {
    /// Constructs a new `Motherboard` with space for `max_devices` devices.
    fn new(max_devices: usize) -> Motherboard {
        Motherboard {
            devices: Vec::with_capacity(max_devices),
        }
    }

    /// The number of slots this motherboard has
    fn num_slots(&self) -> usize {
        self.devices.capacity()
    }

    /// The number of slots filled on this motherboard
    fn slots_filled(&self) -> usize {
        self.devices.len()
    }

    /// Whether or not the motherboard of the device is filled.
    fn is_full(&self) -> bool {
        self.slots_filled() >= self.num_slots()
    }

    /// Adds a new device to the moherboard.
    ///
    /// # Panics
    ///
    /// This method panics if all of the motherboard expansion slots are full.
    fn add_device(&mut self, device: Device) -> Result<(), &'static str>{
        if self.is_full() {
            Err("Cannot add device. Motherboard is full.")
        } else {
            self.devices.push(device);
            Ok(())
        }
    }
}

// CFFI

/// Contructs a pointer to a new `Motherboard` with `max_devices` capacity.
///
/// # Safety
///
/// This function will implicitly convert a `Box<Motherboard>` to a `void*` pointer if
/// called from C or another language. In order to make sure that this memory is
/// dealocated, it should always be passed to `bscomp_motherboard_destroy`.
#[no_mangle]
pub extern fn bscomp_motherboard_new(max_devices: u32) -> Box<Motherboard> {
    Box::new(Motherboard::new(max_devices as usize))
}

/// Destroys a motherboard constructed by `bscomp_motherboard_new`
///
/// Any motherboard created by this module must be passed in here to have its resources
/// released.
///
/// # Safety
///
/// This function implicitly converts a pointer to a `Box<Motherboard>`. You must ensure
/// that the pointer passed into it was initially a valid `Box<Motherboard>`, created by
/// `bscomp_motherboard_new`. Passing anything else is undefined behavior.
#[no_mangle]
pub extern fn bscomp_motherboard_destroy(mb: Box<Motherboard>) {
    drop(mb);
}

/// Gets the number of slots total in the motherboard.
///
/// # Safety
///
/// The motherboard must be a valid motherboard created with `bscomp_motherboard_new`. If
/// the motherboard is null, the result is 0. For anything else, the behavior is
/// undefined.
#[no_mangle]
pub extern fn bscomp_motherboard_num_slots(mb: *mut Motherboard) -> u32 {
    if mb.is_null() {
        0
    } else {
        unsafe { (*mb).num_slots() as u32 }
    }
}

/// Gets the number of slots filled in the motherboard.
///
/// # Safety
///
/// The motherboard must be a valid motherboard created with `bscomp_motherboard_new`. If
/// the motherboard is null, the result is 0. For anything else, the behavior is
/// undefined.
#[no_mangle]
pub extern fn bscomp_motherboard_slots_filled(mb: *mut Motherboard) -> u32 {
    if mb.is_null() {
        0
    } else {
        unsafe { (*mb).slots_filled() as u32 }
    }
}

/// Tests if all the slots on the motherboard a full.
///
/// # Safety
///
/// The motherboard must be a valid motherboard created with `bscomp_motherboard_new`. If
/// the motherboard is null, the result is 1. For anything else, the behavior is
/// undefined.
#[no_mangle]
pub extern fn bscomp_motherboard_is_full(mb: *mut Motherboard) -> i32 {
    if mb.is_null() {
        1
    } else {
        unsafe { (*mb).is_full() as i32 }
    }
}

/// Add a device to the given mother board.
///
/// Returns an error code indicating if the addition is successful. It fails if either of
/// the pointers is null or the motherboard is already full.
///
/// # Safety
///
/// The motherboard must be a motherboard allocated with `bscomp_motherboard_new`.
///
/// The device passed in will be copied, so the caller does not need to keep a pointer to
/// the device. However, it is the responsibility of the caller to enure that any function
/// pointers included in the device live longer than the motherboard.
#[no_mangle]
pub extern fn bscomp_motherboard_add_device(mb: *mut Motherboard, device: *mut Device) -> i32 {
    if mb.is_null() {
        return -1;
    }
    if device.is_null() {
        return -2;
    }

    let mb = unsafe { &mut *mb };
    let device = unsafe { &mut *device };

    match mb.add_device(*device) {
        Ok(_) => 0,
        Err(_) => -3,
    }
}
