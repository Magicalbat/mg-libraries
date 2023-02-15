use crate::ArenaResult;
use mga_sys::{
    mga_create, mga_destroy, mga_get_error, mga_push, mga_push_zero, mga_reset, MGADesc,
    MGAErrorCode, MGArena,
};
use std::mem;

pub struct UnsafeArena {
    inner: *mut MGArena,
}

impl UnsafeArena {
    pub fn new(max_size: u64) -> ArenaResult<Self> {
        assert!(
            max_size > 0,
            "Underlying C implementation will segfault if size is 0."
        );

        let desc = MGADesc {
            desired_max_size: max_size,
            ..Default::default()
        };

        let arena = unsafe { mga_create(&desc as *const MGADesc) };

        let potential_error = unsafe { mga_get_error(arena) };

        match potential_error.code {
            MGAErrorCode::None => {}
            _ => return Err(potential_error),
        }

        Ok(UnsafeArena { inner: arena })
    }

    pub unsafe fn alloc<T>(&mut self, data: T) -> *mut T {
        let ptr = mga_push(self.inner, mem::size_of::<T>() as u64) as *mut T;

        *ptr = data;

        ptr
    }

    pub unsafe fn alloc_zeroed<T>(&mut self) -> *mut T {
        mga_push_zero(self.inner, mem::size_of::<T>() as u64) as *mut T
    }

    pub unsafe fn alloc_slice<T: Copy>(&mut self, data: &[T]) -> *mut [T] {
        let size = mem::size_of::<T>() * data.len();
        let ptr = mga_push(self.inner, size as u64) as *mut T;

        std::ptr::copy(data.as_ptr(), ptr, data.len());

        std::ptr::slice_from_raw_parts_mut(ptr, data.len())
    }

    pub unsafe fn alloc_zeroed_slice<T>(&mut self, len: usize) -> *mut [T] {
        let size = mem::size_of::<T>() * len;
        let ptr = mga_push_zero(self.inner, size as u64) as *mut T;

        std::ptr::slice_from_raw_parts_mut(ptr, len)
    }

    pub unsafe fn reset(&mut self) {
        mga_reset(self.inner);
    }
}

impl Drop for UnsafeArena {
    fn drop(&mut self) {
        unsafe {
            mga_destroy(self.inner);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const ARENA_SIZE: u64 = crate::mib_to_bytes(1);

    fn create_arena() -> UnsafeArena {
        UnsafeArena::new(ARENA_SIZE).unwrap()
    }

    #[test]
    fn new() {
        // Create and drop arena at the end of scope
        let _arena = create_arena();
    }

    #[test]
    #[should_panic]
    fn new_0_sized_arena() {
        let _arena = UnsafeArena::new(0).unwrap();
    }

    #[test]
    fn alloc() {
        unsafe {
            let mut arena = create_arena();

            let data = arena.alloc(13_u8);

            assert_eq!(*data, 13);
            *data = 45;
            assert_eq!(*data, 45);
        }
    }

    #[test]
    fn alloc_struct() {
        #[derive(Debug, Clone, PartialEq)]
        struct Foo {
            x: u32,
            y: bool,
        }

        unsafe {
            let mut arena = create_arena();

            let original = Foo {
                x: u32::MAX / 2,
                y: false,
            };

            let data = arena.alloc(original.clone());

            assert_eq!(*data, original);
            (*data).y = true;
            assert!((*data).y);
        }
    }

    #[test]
    fn alloc_array() {
        unsafe {
            let mut arena = create_arena();

            let original: [u32; 3] = [3, 5, 7];
            let data = arena.alloc(original.clone());

            assert_eq!(*data, original);
            (*data)[1] = 0;
            assert_eq!(*data, [3, 0, 7]);
        }
    }

    #[test]
    fn alloc_zeroed() {
        unsafe {
            let mut arena = create_arena();

            let data = arena.alloc_zeroed::<u32>();

            assert_eq!(*data, 0);
            *data = 1;
            assert_eq!(*data, 1);
        }
    }

    #[test]
    fn alloc_slice() {
        unsafe {
            let mut arena = create_arena();

            let my_slice = &[3, 3, 3, 5];
            let data = arena.alloc_slice(my_slice);

            assert_eq!(*data, my_slice[..]);
            assert_eq!((*data).len(), 4);
            (*data)[0] = 5;
            assert_eq!(*data, [5, 3, 3, 5]);
        }
    }

    #[test]
    fn alloc_zeroed_slice() {
        unsafe {
            let mut arena = create_arena();

            let data = arena.alloc_zeroed_slice::<u8>(7);

            assert_eq!(*data, [0_u8; 7]);
        }
    }

    #[test]
    fn alloc_tiny_arena() {
        unsafe {
            let mut arena = UnsafeArena::new(1).unwrap();

            let byte = arena.alloc(1_u8);

            assert_eq!(*byte, 1);
            *byte = 5;
            assert_eq!(*byte, 5);
        }
    }
}
