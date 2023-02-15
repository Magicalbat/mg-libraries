# mga-sys

These are the raw bindings to the [`mg_arena`](https://github.com/Magicalbat/mg_arena), a single-header C library for memory management with arenas by [Magicalbat](https://github.com/Magicalbat).

It is recommended to use `mga` instead of `mgs-sys` in applications, since the first is intended to be used idiomatically in Rust. (The latter correlates 1-to-1 with the C library.)

# Get started

The main type of this library is [`MGArena`]. To create one, use [`mga_create`] and a [`MGADesc`].

```rust
use mga_sys::*;
use std::mem::size_of;

// We're using C bindings, it's all unsafe from here
unsafe {
    // Create a new memory with 4 MiB of space
    let desc = MGADesc {
        desired_max_size: mga_mib(4),
        ..Default::default()
    };

    // Returns a pointer to the new arena
    let arena = mga_create(&desc as *const MGADesc);

    // `arena` may be a null pointer, so let's check for errors.
    let potential_error = mga_get_error(arena);

    match potential_error.code {
        // No errors, let's move on!
        MGAErrorCode::None => {},
        _ => panic!("Error creating arena: {:?}", potential_error),
    }

    // Allocate a new u32
    let num = mga_push(arena, size_of::<u32>() as u64) as *mut u32;

    // Make sure to set the value before reading it!
    *num = 4;

    assert_eq!(*num, 4);

    // We can zero-out the allocated memory so we can read it first
    let my_array = mga_push_zero(arena, size_of::<[u8; 3]>() as u64) as *mut [u8; 3];

    assert_eq!(*my_array, [0, 0, 0]);

    // Make sure to destroy the arena after you're done using it!
    mga_destroy(arena);
}
```
