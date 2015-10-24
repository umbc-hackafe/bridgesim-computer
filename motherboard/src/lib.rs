extern crate libc;

// Rusty Section

pub struct Device;

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
    pub fn new(max_devices: usize) -> Motherboard {
        Motherboard {
            devices: Vec::with_capacity(max_devices),
        }
    }
}

// CFFI

#[no_mangle]
// Replace this function with the below when into_raw stabilizes
/// Contructs a pointer to a new `Motherboard` with `max_devices` capacity.
///
/// # Safety
///
/// This function will implicitly convert a `Box<Motherboard>` to a `void*` pointer if
/// called from C or another language. In order to make sure that this memory is
/// dealocated, it should always be passed to `bscomp_motherboard_destroy`.
pub extern fn bscomp_motherboard_new(max_devices: libc::size_t) -> Box<Motherboard> {
    Box::new(Motherboard::new(max_devices as usize))
}
// pub extern fn bscomp_motherboard_new(max_devices: libc::size_t) -> *mut Motherboard {
//     Box::into_raw(Box::new(Motherboard::new(max_devices as usize)))
// }

#[no_mangle]
// Replace this function with the below when from_raw stabilizes.
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
pub extern fn bscomp_motherboard_destroy(mb: Box<Motherboard>) {
    drop(mb);
}
// pub extern fn bscomp_motherboard_destroy(mb: *mut Motherboard) {
//     if !mb.is_null() {
//         unsafe { drop(Box::from_raw(mb)); }
//     }
// }
