use mga_sys::*;
use std::mem;

fn main() {
    unsafe {
        let desc = MGADesc {
            max_size: mga_mib(4),
            desired_block_size: 8,

            page_size: 0,
            align: 0,
        };

        let arena = mga_create(&desc as *const MGADesc);

        let nums = mga_push_zero(arena, (mem::size_of::<u32>() * 64) as u64) as *mut u32;

        for i in 0..64 {
            *nums.add(i) = i as u32
        }

        assert_eq!(*nums, 0);
        assert_eq!(*nums.add(3), 3);
        assert_eq!(*nums.add(63), 63);

        let _data = mga_push_zero(arena, (mem::size_of::<char>() * 128) as u64) as *mut char;

        mga_destroy(arena);
    }
}
