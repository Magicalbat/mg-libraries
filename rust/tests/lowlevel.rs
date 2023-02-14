use mga::{self, lowlevel::UnsafeArena};

#[test]
fn main() {
    unsafe {
        let mut arena = UnsafeArena::new(mga::mib_to_bytes(1));

        let nums = arena.alloc_zeroed_slice::<u64>(64);

        for i in 0..(*nums).len() {
            (*nums)[i] = i as u64;
        }

        assert_eq!((*nums)[0], 0);
        assert_eq!((*nums)[3], 3);
        assert_eq!((*nums)[63], 63);

        // De-allocate nums, make sure not to use the it after this!
        arena.reset();

        // Arena automatically gets freed here.
    }
}
