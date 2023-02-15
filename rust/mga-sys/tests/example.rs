use mga_sys::*;
use std::{
    ffi::{c_char, CStr},
    mem,
};

// A callback function that prints the error to stderr
unsafe extern "C" fn err_callback(code: MGAErrorCode, msg: *const c_char) {
    // Convert to CStr, so we can print out the message
    let msg = unsafe { CStr::from_ptr(msg) };
    eprintln!("MGA Error {:?}: {}", code, msg.to_string_lossy());
}

// An example struct to test allocation
struct Foo {
    x: f32,
    y: u32,
}

#[test]
fn main() {
    unsafe {
        // Parameters for arena creation, with defaults being filled in
        let desc = MGADesc {
            desired_max_size: mga_mib(4),
            desired_block_size: mga_kib(128) as u32,
            error_callback: Some(err_callback),
            ..Default::default()
        };

        let arena = mga_create(&desc as *const MGADesc);

        println!("Pos before push: {}", mga_get_pos(arena));

        // Allocate [0_u32; 64] in the arena, return a pointer to the first element
        let nums = mga_push_zero(arena, (mem::size_of::<u32>() * 64) as u64) as *mut u32;

        for i in 0..64 {
            // If this were a slice / array, this would be the same as:
            // nums[i] = i as u32
            *nums.add(i) = i as u32
        }

        println!("Pos after push: {}", mga_get_pos(arena));

        assert_eq!(*nums, 0);
        assert_eq!(*nums.add(3), 3);
        assert_eq!(*nums.add(63), 63);

        // Frees all allocatings in memory, make sure not to use `nums` after this!
        mga_reset(arena);

        // Allocate the memory to store 1 Foo
        let foo = mga_push(arena, mem::size_of::<Foo>() as u64) as *mut Foo;

        // Make sure to set `foo` before accessing it!
        *foo = Foo { x: -1.0, y: 67 };

        assert_eq!((*foo).x, -1.0);
        assert_eq!((*foo).y, 67);

        // It's good practice to destroy the arena when you're done with it, so you don't leak
        // memory
        mga_destroy(arena);
    }
}
