extern crate libc;

// Rusty Section

pub struct Device;

pub struct Motherboard {
    max_devices: usize,
    mapped_devices: Vec<Device>,
}

impl Motherboard {
    fn new(max_devices: usize) -> Motherboard {
        Motherboard {
            max_devices: max_devices,
            mapped_devices: Vec::with_capacity(max_devices),
        }
    }
}

// CFFI

#[no_mangle]
// Replace this function when into_raw stabilizes
// pub extern fn bscomp_motherboard_new(max_devices: libc::size_t) -> *mut Motherboard {
//     Box::into_raw(Box::new(Motherboard::new(max_devices as usize)))
// }
pub extern fn bscomp_motherboard_new(max_devices: libc::size_t) -> Box<Motherboard> {
    Box::new(Motherboard::new(max_devices as usize))
}

#[no_mangle]
// Replace this function when from_raw stabilizes.
// pub extern fn bscomp_motherboard_destroy(mb: *mut Motherboard) {
//     if !mb.is_null() {
//         unsafe { drop(Box::from_raw(mb)); }
//     }
// }
pub extern fn bscomp_motherboard_destroy(mb: Box<Motherboard>) {
    drop(mb);
}
