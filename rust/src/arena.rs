use crate::sys;
use std::marker::PhantomData;

pub fn with_arena<F>(max_size: u64, func: F)
where
    F: FnOnce(Arena<'_>),
{
    let arena = unsafe { Arena::new(max_size) };
    func(arena);

    // Arena is dropped here
}

pub struct Arena<'a> {
    inner: *mut sys::MGArena,
    phantom: PhantomData<&'a ()>,
}

impl<'a> Arena<'a> {
    pub unsafe fn new(max_size: u64) -> Self {
        let desc = sys::MGADesc {
            max_size: max_size,
            pages_per_block: 8,

            page_size: 0,
            align: 0,
        };

        let arena = sys::mga_create(&desc as *const sys::MGADesc);

        Arena {
            inner: arena,
            phantom: PhantomData,
        }
    }
}

impl Drop for Arena<'_> {
    fn drop(&mut self) {
        unsafe { crate::sys::mga_destroy(self.inner) }
    }
}
