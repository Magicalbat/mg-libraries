use mga_sys::{mga_create, mga_destroy, mga_push, mga_push_zero, MGADesc, MGArena};
use std::mem;

pub struct UnsafeArena {
    inner: *mut MGArena,
}

impl UnsafeArena {
    pub unsafe fn new(size: u64) -> Self {
        let desc = MGADesc {
            max_size: size,
            ..Default::default()
        };

        let arena = mga_create(&desc as *const MGADesc);

        UnsafeArena { inner: arena }
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

    const ARENA_SIZE: u64 = mga_sys::mga_mib(4);

    fn create_arena() -> UnsafeArena {
        unsafe { UnsafeArena::new(ARENA_SIZE) }
    }

    #[test]
    fn new() {
        // Create and drop arena at the end of scope
        let _arena = create_arena();
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
}
