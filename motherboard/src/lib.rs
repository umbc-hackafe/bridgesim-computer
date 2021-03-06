extern crate libc;

use libc::c_void;
use std::mem;
use std::sync::mpsc;
use std::sync::Mutex;
use std::thread;

// Rusty Section

/// Enum to send to the motherboard to change state.
enum MotherboardInterrupt {
    Halt,
    Reboot,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct MotherboardConfig {
    pub max_devices: u32,
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

    /// This should be a unique identifier for the type of device.
    ///
    /// The lower 32 bits of this type are reserved for flags giving information about the
    /// device.
    pub device_type: u64,

    /// This should be a fully unique identifier for the device within its type.
    pub device_id: u32,

    /// Indicates the size of exported memory
    pub export_memory_size: u32,

    /// Function to load a byte from the device's memory.
    ///
    /// Should take four arguments:
    /// - The device pointer
    /// - The (local) address to read from
    /// - The number of bytes to read
    /// - A pointer to the memory to store the read bytes in (assumed to be large enough).
    ///
    /// Reads should simply ignore invalid memory addresses, and fill as much as they can.
    pub load_bytes: Option<extern fn(*mut c_void, u32, u32, *mut u8) -> i32>,
    /// Function to save a byte to the device's memory.
    ///
    /// Should take four arguments:
    /// - The device pointer
    /// - The (local) address to write to
    /// - The number of bytes to write
    /// - A pointer to the memory to read the written bytes from.
    ///
    /// Writes should simply ignore invalid memory addresses, and fill as much as they
    /// can.
    pub write_bytes: Option<extern fn(*mut c_void, u32, u32, *const u8) -> i32>,

    /// Function to use to initialize the device before booting.
    ///
    /// Should take one argument: the device pointer.
    pub init: Option<extern fn(*mut c_void) -> i32>,

    /// Function used to reset device to startup state if rebooting. Also called on
    /// initial boot.
    pub reset: Option<extern fn(*mut c_void) -> i32>,

    /// Function used to cleanup resources before shutting down.
    ///
    /// Should not rely on the presence of other devices, as these may have already been
    /// cleaned up.
    pub cleanup: Option<extern fn(*mut c_void) -> i32>,

    /// Optional function to use to start the device controller.
    ///
    /// Should take one argument: the device pointer. This function should run until the
    /// device is shut down.
    ///
    /// Any device which implements this function is expected to provide a `halt`
    /// function which shuts down the boot loop.
    pub boot: Option<extern fn(*mut c_void) -> i32>,

    /// Optional function to use to halt this device controller.
    ///
    /// Should take one argument: the device pointer. This function should cause boot to
    /// stop running.
    pub halt: Option<extern fn(*mut c_void) -> i32>,

    /// Optional function to use to send an interrupt code to the device.
    ///
    /// Should take two arguments: the device pointer, and the interrupt code.
    pub interrupt: Option<extern fn(*mut c_void, u32) -> i32>,

    /// Registers the motherboard's functions with this device
    ///
    /// Should take three arguments: the device pointer, a pointer to the motherboard, and
    /// a pointer to the table of motherboard functions. The motherboard pointer should be
    /// saved for use in callbacks, but motherboard device table pointer should have its
    /// contents copied; the pointer to it should not be saved.
    pub register_motherboard: Option<extern fn(
        *mut c_void, *mut Motherboard, *mut MotherboardFunctions) -> i32>,
}

impl Device {
    /// Tells if the device should be memory mapped.
    fn maps_memory(&self) -> bool {
        ((1<<0) & self.device_type) != 0
    }
}

/// Struct for passing the motherboard's function collection to modules.
#[repr(C)]
#[derive(Copy, Clone)]
#[allow(raw_pointer_derive)]
pub struct MotherboardFunctions {
    /// Callback for a device to read a byte of memory.
    ///
    /// Should take four arguments:
    /// - The motherboard
    /// - The (global) address to read from
    /// - The number of bytes to read
    /// - An array of bytes to write to (assumed to be large enough)
    pub read_bytes: Option<extern fn(*mut Motherboard, u64, u32, *mut u8) -> i32>,

    /// Callback for a device to write a byte of memory.
    ///
    /// Should take four arguments:
    /// - The motherboard
    /// - The (global) address to write to
    /// - The number of bytes to write
    /// - An array of bytes to read from (assumed to be large enough)
    pub write_bytes: Option<extern fn(*mut Motherboard, u64, u32, *const u8) -> i32>,

    /// Callback for a device to send an interrupt to another device.
    ///
    /// Should take three arguments: the motherboard, the index in the device table of the
    /// target device, and the interrupt code to send.
    pub send_interrupt: Option<extern fn(*mut Motherboard, u32, u32) -> i32>,
}

/// Represents a motherboard in the bridgesim computer.
///
/// The motherboard has a fixed set of spaces for `Device` objects, which are anything
/// which can do or hold something. The motherboard is the core of the bridgesim computer
/// and is responsible for booting attached devices and memory access control.
pub struct Motherboard {
    devices: Vec<Device>,
    ram_mappings: Vec<usize>,
    deviceinfo_memory: Vec<u8>,
    interrupt_chan: Mutex<Option<mpsc::Sender<MotherboardInterrupt>>>,
}

impl Motherboard {
    /// Constructs a new `Motherboard` with space for `max_devices` devices.
    fn new(max_devices: usize) -> Motherboard {
        Motherboard {
            devices: Vec::with_capacity(max_devices),
            ram_mappings: Vec::new(),
            deviceinfo_memory: Vec::new(),
            interrupt_chan: Mutex::new(None),
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
    fn add_device(&mut self, device: Device) -> Result<(), &'static str> {
        if self.is_full() {
            Err("Cannot add device. Motherboard is full.")
        } else {
            self.devices.push(device);
            Ok(())
        }
    }

    /// Load some bytes from the appropriate device.
    ///
    /// Maps the upper portion of the address to a device index and the lower portion to
    /// addresses inside of that device. Attempts to set the device to the correct amount
    /// of memory from the correct device. If the device index is the last index, load
    /// from the motherboard information section instead.
    fn load_bytes(&self, addr: u64, dest: &mut [u8]) -> i32 {
        // Shift down to get the device index.
        let ram_index = (addr >> 32) as u32;

        // Size-check -- it is a simulator error to try to read more than u32::max_value
        // bytes.
        if dest.len() > (u32::max_value() as usize) {
            return -10
        }

        let start_addr = addr as u32;

        if ram_index == !0u32 {
            // Interpret the end address as an exclusive over the length of the
            // read. Potential for off-by-one errors, but makes it so all reads have at least
            // one byte and makes it so no byte is unmappable.
            let end_addr = (start_addr as u64) + (dest.len() as u64);

            let end_addr = std::cmp::min(end_addr, self.deviceinfo_memory.len() as u64)
                as u32;

            // Loop over local memory writing the appropriate bytes to the read.
            for (d, s) in (start_addr as usize..end_addr as usize).enumerate() {
                dest[d] = self.deviceinfo_memory[s];
            }
            0
        } else if (ram_index as usize) < self.ram_mappings.len() {
            // Find the appropriate device.
            let device_index = self.ram_mappings[ram_index as usize];
            let device = self.devices[device_index];

            // Even though devices are expected to ignore invalid reads anyway, limit to
            // mapped memory.
            let read_size = std::cmp::min(dest.len() as u32, device.export_memory_size);

            match device.load_bytes {
                Some(load_bytes) => {
                    load_bytes(device.device, start_addr, read_size, &mut dest[0])
                },
                // Device does not provide read-bytes. This is not a simulator error, it's
                // an invalid operation which would have to be prevented by the operating
                // system.
                None => 0,
            }
        } else {
            // Do nothing if the memory is not mapped.
            0
        }
    }

    /// Write bytes to the appropriate device.
    fn write_bytes(&self, addr: u64, source: &[u8]) -> i32 {
        // Shift down to get the device index.
        let ram_index = (addr >> 32) as u32;

        // Size-check -- it is a simulator error to try to read more than u32::max_value
        // bytes.
        if source.len() > (u32::max_value() as usize) {
            return -10;
        }

        let start_addr = addr as u32;

        if ram_index == !0u32 {
            // Ignore writes to motherboard memory.
            0
        } else if (ram_index as usize) < self.ram_mappings.len() {
            // find the appropriate device.
            let device_index = self.ram_mappings[ram_index as usize];
            let device = self.devices[device_index];

            // Clamp to mapped memory (even though devices should ignore invalid writes).
            let read_size = std::cmp::min(
                source.len() as u32, device.export_memory_size - start_addr);

            match device.write_bytes {
                Some(load_bytes) => {
                    load_bytes(device.device, start_addr, read_size, &source[0])
                },
                // Device does not provide write-bytes. This is not a simulator error, it's
                // an invalid operation which would have to be prevented by the operating
                // system.
                None => 0,
            }
        } else {
            // Ignore writes to unmapped memory.
            0
        }
    }

    /// Send an interrupt to some device.
    ///
    /// If the device is `0xffffffff`, send to the motherboard.
    fn send_interrupt(&self, device: u32, code: u32) -> i32 {
        if device == !0u32 {
            // ignore unknow interrupts.
            if code > 1 {
                return 0;
            }

            let ic = self.interrupt_chan.lock().unwrap();
            match *ic {
                Some(ref ch) => {
                    let ch = (*ch).clone();
                    let code = match code {
                        0 => MotherboardInterrupt::Halt,
                        1 => MotherboardInterrupt::Reboot,
                        // We have assured this never happens above, so just use halt as
                        // the default.
                        _ => MotherboardInterrupt::Halt,
                    };

                    match ch.send(code) {
                        Ok(_) => 0,
                        // Dissconnected = not booted
                        Err(_) => -1,
                    }
                },
                // No channel = not booted
                None => -1,
            }
        } else if (device as usize) < self.devices.len() {
            let device = self.devices[device as usize];

            match device.interrupt {
                Some(interrupt) => interrupt(device.device, code),
                None => 0,
            }
        } else {
            0
        }
    }

    /// Start the computer
    ///
    /// This function inits an maps each device, then starts calling tick on them until a
    /// shutdown message is received on the machine's shutdown channel.
    fn boot(&mut self) -> Result<(), &'static str> {
        // Set a channel to use to trigger shutdown.
        let (interrupt_chan_tx, interrupt_chan) =  mpsc::channel();
        {
            let mut ic = self.interrupt_chan.lock().unwrap();
            *ic = Some(interrupt_chan_tx.clone());
        }

        let mut mbfuncs = MotherboardFunctions {
            read_bytes: Some(bscomp_motherboard_load_bytes),
            write_bytes: Some(bscomp_motherboard_write_bytes),
            send_interrupt: Some(bscomp_motherboard_send_interrupt),
        };

        let sp: *mut Motherboard = self;

        println!("Begin Booting.");

        // Check for required functions on devices.
        for device in self.devices.iter() {

            if device.maps_memory()
                && (device.load_bytes.is_none() || device.write_bytes.is_none()) {
                    return Err("A memory mapping device did not provide all of \
                                the required functions for memory mapping.");
                }

            if device.boot.is_some() && device.halt.is_none() {
                return Err("A device with a boot method did not provide the required \
                            halt method.");
            }

        }

        println!("Checked device functions.");

        for device in self.devices.iter() {

            match device.register_motherboard {
                Some(register) => { register(device.device, sp, &mut mbfuncs); },
                None => {},
            }
        }

        println!("Registered with devices.");

        // Reset ram mappings in case we've booted before.
        self.ram_mappings.clear();

        // Place an entry in the appropriate memory-mapping table, mapping the index in
        // that table back to the device being mapped.
        for (i, device) in self.devices.iter().enumerate() {
            if device.maps_memory() {
                self.ram_mappings.push(i)
            }
        }

        // Compute according to the layout described in memoryhandling.md the address of
        // each table which will be in the system's low memory.
        let ramstart: u64 = 2 * mem::size_of::<u64>() as u64 + 0xffffffff00000000u64;
        let devstart: u64 = ramstart
            + (self.ram_mappings.len() as u64 + 1) * mem::size_of::<u32>() as u64;

        // Clean out any old mapping to ensure we start fresh.
        self.deviceinfo_memory.clear();

        // Load the low memory table according to the layout described in memoryhandling.md
        self.deviceinfo_memory.extend(
            unsafe { mem::transmute::<u64, [u8; 8]>(ramstart) }.iter());
        self.deviceinfo_memory.extend(
            unsafe { mem::transmute::<u64, [u8; 8]>(devstart) }.iter());

        self.deviceinfo_memory.extend(unsafe {
            mem::transmute::<u32, [u8; 4]>(self.ram_mappings.len() as u32)
        }.iter());
        for mmap in self.ram_mappings.iter() {
            self.deviceinfo_memory.extend(unsafe {
                mem::transmute::<u32, [u8; 4]>(self.devices[*mmap].export_memory_size)
            }.iter());
        }

        self.deviceinfo_memory.extend(unsafe {
            mem::transmute::<u32, [u8; 4]>(self.devices.len() as u32)
        }.iter());

        for (i, device) in self.devices.iter().enumerate() {
            unsafe {
                self.deviceinfo_memory.extend(
                    mem::transmute::<u64, [u8; 8]>(device.device_type).iter());
                self.deviceinfo_memory.extend(
                    mem::transmute::<u32, [u8; 4]>(device.device_id).iter());
            }

            self.deviceinfo_memory.extend(unsafe {
                mem::transmute::<u32, [u8; 4]>(
                    match self.ram_mappings.binary_search(&i) {
                        Ok(i) => i,
                        Err(_) => 0,
                    } as u32)
            }.iter());
        }

        println!("Prepared Motherboard Memory.");

        for device in self.devices.iter() {
            match device.init {
                // errors will just be ignored....
                Some(init) => { init(device.device); },
                None => {},
            };
        }

        println!("Initialized devices.");

        loop {
            for device in self.devices.iter() {
                match device.reset {
                    Some(reset) => { reset(device.device); },
                    None => {},
                };
            }

            println!("Reset devices.");

            // Boot the devices.
            let mut thread_handles = Vec::new();

            for device in self.devices.iter() {
                match device.boot {
                    Some(boot) => {
                        let boot = boot;

                        // Coherce this pointer to a reference because pointers are not
                        // "send". Then send the reference and coherce it back to a
                        // pointer.
                        let device = unsafe { &mut *device.device };
                        let thread_handle = thread::spawn(move || {
                            let device = device;
                            boot(device);
                        });

                        thread_handles.push(thread_handle);
                    },
                    None => {},
                }
            }

            let action = match interrupt_chan.recv() {
                Ok(action) => action,
                Err(_) => MotherboardInterrupt::Halt,
            };

            // Order all devices to halt.
            for device in self.devices.iter() {
                if device.boot.is_some() {
                    // Have already assured that hatl is Some before booting.
                    device.halt.unwrap()(device.device);
                }
            }

            for handle in thread_handles.into_iter() {
                let _ = handle.join();
            }

            match action {
                MotherboardInterrupt::Halt => break,
                _ => {},
            };
        }

        println!("Halted.");

        for device in self.devices.iter() {
            match device.cleanup {
                Some(cleanup) => { cleanup(device.device); },
                None => {},
            };
        }

        println!("Cleaned up devices.");

        {
            let mut ic = self.interrupt_chan.lock().unwrap();
            *ic = None;
        }

        println!("Shutdown.");

        Ok(())
    }

    /// Halt the machine. This method is threadsafe!
    fn halt(&self) -> i32 {
        let ic = self.interrupt_chan.lock().unwrap();
        match *ic {
            Some(ref ch) => {
                let ch = (*ch).clone();
                match ch.send(MotherboardInterrupt::Halt) {
                    Ok(_) => 0,
                    // Dissconnected = not booted
                    Err(_) => -1,
                    }
            },
            // No channel = not booted
            None => -1,
        }
    }

    /// Reboot the machine. This method is threadsafe!
    fn reboot(&self) -> i32 {
        let ic = self.interrupt_chan.lock().unwrap();
        match *ic {
            Some(ref ch) => {
                let ch = (*ch).clone();
                match ch.send(MotherboardInterrupt::Reboot) {
                    Ok(_) => 0,
                    // Dissconnected = not booted
                    Err(_) => -1,
                    }
            },
            // No channel = not booted
            None => -1,
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
pub extern fn bscomp_motherboard_new(config: *const MotherboardConfig) -> *mut Motherboard {
    if config.is_null() {
        std::ptr::null_mut()
    } else {
        let config = unsafe { &*config };
        Box::into_raw(Box::new(Motherboard::new(config.max_devices as usize)))
    }
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
pub extern fn bscomp_motherboard_destroy(mb: *mut Motherboard) {
    if !mb.is_null() {
        let mb = unsafe { Box::from_raw(mb) };
        drop(mb);
    }
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
pub extern fn bscomp_motherboard_add_device(
    mb: *mut Motherboard, device: *mut Device) -> i32 {

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

/// C-callable load-bytes method
pub extern fn bscomp_motherboard_load_bytes(
    mb: *mut Motherboard, addr: u64, bytes_count: u32, destination: *mut u8) -> i32 {

    if mb.is_null() {
        -1
    } else if destination.is_null() {
        -2
    } else {
        let mb = unsafe { &mut *mb };
        let dest = unsafe {
            std::slice::from_raw_parts_mut(destination, (bytes_count as usize))
        };

        mb.load_bytes(addr, dest)
    }
}

/// C-callable write-bytes method
pub extern fn bscomp_motherboard_write_bytes(
    mb: *mut Motherboard, addr: u64, bytes_count: u32, source: *const u8) -> i32 {

    if mb.is_null() {
        -1
    } else if source.is_null() {
        -2
    } else {
        let mb = unsafe { &mut *mb };
        let src = unsafe {
            std::slice::from_raw_parts(source, (bytes_count as usize))
        };

        mb.write_bytes(addr, src)
    }
}

/// C-callable send-interrupt method
pub extern fn bscomp_motherboard_send_interrupt(
    mb: *mut Motherboard, device: u32, code: u32) -> i32 {

    if mb.is_null() {
        -1
    } else {
        let mb = unsafe { &mut *mb};
        mb.send_interrupt(device, code)
    }
}

/// Boot the motherboard -- hangs until shutdown.
#[no_mangle]
pub extern fn bscomp_motherboard_boot(mb: *mut Motherboard) -> i32 {
    if mb.is_null() {
        -1
    } else {
        let mb = unsafe { &mut *mb };
        match mb.boot() {
            Ok(_) => 0,
            Err(_) => -2,
        }
    }
}

/// Halt the machine
#[no_mangle]
pub extern fn bscomp_motherboard_halt(mb: *mut Motherboard) -> i32 {
    if mb.is_null() { -1 }
    else {
        let mb = unsafe { &mut *mb };
        mb.halt()
    }
}

/// Reboot the machine
#[no_mangle]
pub extern fn bscomp_motherboard_reboot(mb: *mut Motherboard) -> i32 {
    if mb.is_null() { -1 }
    else {
        let mb = unsafe { &mut *mb };
        mb.reboot()
    }
}
