use std::{
    ffi::{c_char, c_void, CStr},
    fmt,
    mem::ManuallyDrop,
};

#[link(name = "mg_arena")]
extern "C" {
    /// Creates a new arena from a [`MGADesc`], returning a pointer to the new arena.
    ///
    /// # Example
    ///
    /// ```
    /// # use mga_sys::*;
    /// # unsafe {
    /// let arena = mga_create(&MGADesc::default() as *const MGADesc);
    ///
    /// // Do stuff
    ///
    /// mga_destroy(arena);
    /// # }
    /// ```
    pub fn mga_create(desc: *const MGADesc) -> *const MGArena;

    /// Destroys an arena and frees its memory.
    ///
    /// See [`mga_create`] for example usage.
    pub fn mga_destroy(arena: *const MGArena);

    /// Returns the last error from the given arena.
    ///
    /// If `arena` is a null pointer (for instance if arena creation failed), this will fall back to
    /// a thread local variable with the most recent implementation.
    pub fn mga_get_error(arena: *const MGArena) -> MGAError;

    /// Returns the current position of an arena.
    pub fn mga_get_pos(arena: *const MGArena) -> u64;

    /// Returns the current size of an arena.
    pub fn mga_get_size(arena: *const MGArena) -> u64;

    /// Returns the block size in an arena.
    pub fn mga_get_block_size(arena: *const MGArena) -> u32;

    /// Returns the alignment of an arena.
    pub fn mga_get_align(arena: *const MGArena) -> u32;

    /// Allocates `size` bytes in the arena, returning a pointer to the beginning of the allocated
    /// memory.
    ///
    /// # Example
    ///
    /// ```
    /// # use mga_sys::*;
    /// # unsafe {
    /// let arena = mga_create(&MGADesc::default() as *const MGADesc);
    ///
    /// // Allocate 1 byte
    /// let data = mga_push(arena, 1) as *mut u8;
    ///
    /// unsafe {
    ///     *data = 3;
    ///     assert_eq!(*data, 3);
    /// }
    ///
    /// mga_destroy(arena);
    /// # }
    /// ```
    pub fn mga_push(arena: *const MGArena, size: u64) -> *mut c_void;

    /// Same as [mga_push], but it zeroes out the allocated memory first.
    ///
    /// # Example
    ///
    /// ```
    /// # use mga_sys::*;
    /// # unsafe {
    /// let arena = mga_create(&MGADesc::default() as *const MGADesc);
    ///
    /// let data = mga_push_zero(arena, 1) as *mut u8;
    ///
    /// // mga_push_zero guarantees data is 0, while mga_push does not.
    /// assert_eq!(*data, 0);
    ///
    /// mga_destroy(arena);
    /// # }
    /// ```
    pub fn mga_push_zero(arena: *const MGArena, size: u64) -> *mut c_void;

    /// Frees `size` bytes in the arena.
    ///
    /// # Warning
    ///
    /// Due to memory alignment, this may not always act as expected. Make sure you know what you're
    /// doing!
    pub fn mga_pop(arena: *const MGArena, size: u64);

    /// Frees all bytes after `pos`, updating the new arena position.
    ///
    /// # Warning
    ///
    /// Due to memory alignment, this may not always act as expected. Make sure you know what you're
    /// doing!
    pub fn mga_pop_to(arena: *const MGArena, pos: u64);

    /// Frees all memory in arena, returning it to its original position.
    ///
    /// Ensure that pointers allocated previously are not used after calling this function.
    pub fn mga_reset(arena: *const MGArena);

    /// Creates a new temporary arena from the given arena.
    pub fn mga_temp_begin(arena: *const MGArena) -> MGATemp;

    /// Destroys the temporary arena, freeing all memory allocated in it.
    pub fn mga_temp_end(temp: MGATemp);
}

/// An arena that you can allocate data on, see [`mga_create`].
#[repr(C)]
pub struct MGArena {
    _pos: u64,

    _size: u64,
    _block_size: u64,
    _align: u32,

    _backend: MGArenaBackend,

    _last_error: MGAError,

    /// The current arena's error callback function.
    ///
    /// For more information, see [`MGAErrorCallback`].
    pub error_callback: MGAErrorCallback,
}

/// Initialization parameters for an [`MGArena`].
///
/// # Example
///
/// ```
/// # use mga_sys::{MGADesc, mga_mib};
/// #
/// let desc = MGADesc {
///     desired_max_size: mga_mib(4),
///     ..Default::default()
/// };
/// ```
///
/// # Note about Default
///
/// This struct implements [`Default`] to fill in unspecified parameters. `desired_max_size` will
/// segfault if set to 0, so the default rounds up to the nearest page size. **Do not depend on
/// this, and always specified `desired_max_size` when creating arenas!**
#[repr(C)]
#[derive(Debug, Clone)]
pub struct MGADesc {
    /// The maximum size of the arena, rounded up to the nearest page size.
    ///
    /// # Warning
    ///
    /// **This will segfault if set to 0!**
    pub desired_max_size: u64,

    /// The size of a block in the arena, rounded up to the nearest page size.
    ///
    /// If using the malloc backend, each node will be a multiple of this block size. If using a
    /// lower-level backend, the entire memory will be a multiple of this block size.
    pub desired_block_size: u32,

    /// The size of memory alignment.
    ///
    /// **The given alignment must be a power of two.** (E.g. `2.pow(x) == align`.) To disable
    /// alignment, set this to 1.
    ///
    /// # Notes
    ///
    /// See [this article](https://developer.ibm.com/articles/pa-dalign/) for the rationality behind alignment.
    pub align: u32,

    /// The error callback function to use.
    ///
    /// For more information, see [`MGAErrorCallback`].
    pub error_callback: MGAErrorCallback,
}

impl Default for MGADesc {
    fn default() -> Self {
        MGADesc {
            // Round up to nearest page size
            desired_max_size: 1,
            desired_block_size: 0,
            align: 0,
            error_callback: None,
        }
    }
}

#[repr(C)]
union MGArenaBackend {
    _malloc_arena: ManuallyDrop<MGAMallocBackend>,
    _reserve_arena: ManuallyDrop<MGAReserveBackend>,
}

#[repr(C)]
#[derive(Debug)]
struct MGAMallocBackend {
    pub cur_node: *const MGAMallocBackend,
}

#[repr(C)]
#[derive(Debug)]
struct MGAMallocNode {
    pub prev: *const MGAMallocNode,
    pub size: u64,
    pub pos: u64,
    pub data: *const u8,
}

#[repr(C)]
#[derive(Debug)]
struct MGAReserveBackend {
    pub commit_pos: u64,
}

/// An error raised by the arena or associated functions.
///
/// For error codes, see [`MGAErrorCode`].
#[repr(C)]
pub struct MGAError {
    /// The kind of error being raised.
    pub code: MGAErrorCode,

    /// A C string for the debug message behind the error.
    pub msg: *const c_char,
}

impl MGAError {
    /// Returns a [`String`] of `msg`, with invalid UTF-8 data replaced using
    /// `CStr::to_string_lossy`.
    pub fn msg_to_string(&self) -> String {
        unsafe { CStr::from_ptr(self.msg).to_string_lossy().into_owned() }
    }
}

// Uses .msg_to_str() instead of default implementation
impl fmt::Debug for MGAError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        Ok(f.debug_struct("MGAError")
            .field("code", &self.code)
            .field("msg", &self.msg_to_string())
            .finish()?)
    }
}

/// An enum used to represent different kinds of errors.
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum MGAErrorCode {
    /// No error occurred.
    None = 0,

    /// The arena failed to initialize.
    InitFailed,

    /// The arena failed to allocate memory with `malloc`.
    MallocFailed,

    /// The `malloc`-based arena ran out of memory.
    OutOfNodes,

    /// The arena failed to commit memory.
    CommitFailed,

    /// The arena position exceeded arena size.
    OutOfMemory,
}

/// A callback function that is called when an error occurs in an arena.
///
/// If this is set to `None` (e.g. the function pointer is null), no error function will be called.
///
/// See [`MGAError`] for function argument documentation. You can also get the error through
/// [`mga_get_error`].
pub type MGAErrorCallback = Option<unsafe extern "C" fn(code: MGAErrorCode, msg: *const c_char)>;

/// A temporary arena, see [`mga_temp_begin`].
#[repr(C)]
#[derive(Debug)]
pub struct MGATemp {
    pub arena: *const MGArena,
    _pos: u64,
}

/// Returns number of bytes for given KiB (1,024 bytes).
pub const fn mga_kib(x: u64) -> u64 {
    x << 10
}

/// Returns number of bytes for given MiB (1,048,576 bytes).
pub const fn mga_mib(x: u64) -> u64 {
    x << 20
}

/// Returns number of bytes for given MiB (1,073,741,824 bytes).
pub const fn mga_gib(x: u64) -> u64 {
    x << 30
}
