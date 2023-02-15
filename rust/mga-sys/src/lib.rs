use std::{
    ffi::{c_char, c_void},
    mem::ManuallyDrop,
};

#[link(name = "mg_arena")]
extern "C" {
    /// Creates a new arena, returning a pointer to it.
    ///
    /// # Example
    ///
    /// ```
    /// # use mga_sys::{mga_create, mga_destroy, MGADesc};
    /// # unsafe {
    /// let arena = mga_create(&MGADesc::default() as *const MGADesc);
    ///
    /// // Do stuff
    ///
    /// mga_destroy(arena);
    /// # }
    /// ```
    pub fn mga_create(desc: *const MGADesc) -> *mut MGArena;

    /// Destroys an arena and frees its memory.
    ///
    /// See [`mga_create`] for example.
    pub fn mga_destroy(arena: *mut MGArena);

    pub fn mga_get_error(arena: *mut MGArena) -> MGAError;

    pub fn mga_get_pos(arena: *mut MGArena) -> u64;
    pub fn mga_get_size(arena: *mut MGArena) -> u64;
    pub fn mga_get_block_size(arena: *mut MGArena) -> u32;
    pub fn mga_get_align(arena: *mut MGArena) -> u32;

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
    pub fn mga_push(arena: *mut MGArena, size: u64) -> *mut c_void;

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
    /// assert_eq!(*data, 0);
    ///
    /// mga_destroy(arena);
    /// # }
    /// ```
    pub fn mga_push_zero(arena: *mut MGArena, size: u64) -> *mut c_void;

    /// Frees `size` bytes in the arena.
    pub fn mga_pop(arena: *mut MGArena, size: u64);
    /// Frees all bytes after `pos`.
    pub fn mga_pop_to(arena: *mut MGArena, pos: u64);

    pub fn mga_reset(arena: *mut MGArena);

    /// Begins a temporary arena with the given arena.
    pub fn mga_temp_begin(arena: *mut MGArena) -> MGATemp;
    /// Ends a temporary arena with the given arena.
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
    pub error_callback: MGAErrorCallback,
}

impl MGArena {
    pub fn size(&self) -> u64 {
        self._size
    }
}

/// An arena descriptor, used to pass information for building the arena. This struct implements
/// [`Default`], which you can use to fill in default arguments.
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
#[repr(C)]
#[derive(Debug, Clone)]
pub struct MGADesc {
    /// Maximum size of the arena, must be set or else the arena will be unable to allocate
    /// anything.
    pub desired_max_size: u64,

    /// The amount of pages per block in the arena, defaults to `min(max_size / page_size, 8)`.
    pub desired_block_size: u32,

    /// The alignment, defaults to the size of a pointer `sizeof(void*)`.
    pub align: u32,

    pub error_callback: MGAErrorCallback,
}

// Technically could be derived, but I'd rather be explicit
impl Default for MGADesc {
    fn default() -> Self {
        MGADesc {
            desired_max_size: mga_mib(1),
            desired_block_size: 0,
            align: 0,
            error_callback: None,
        }
    }
}

/// A union that represents different backend kinds.
///
/// # Implementation Note
///
/// I added [`ManuallyDrop`] because it fixed a random error I was getting. I should go back later
/// and try to find a better solution that I actually understand.
#[repr(C)]
pub union MGArenaBackend {
    _malloc_arena: ManuallyDrop<MGAMallocBackend>,
    _reserve_arena: ManuallyDrop<MGAReserveBackend>,
}

#[repr(C)]
#[derive(Debug)]
pub struct MGAMallocBackend {
    pub first: *mut MGAMallocNode,
    pub last: *mut MGAMallocNode,
    pub num_nodes: u32,
}

/// Used by [`MGAMallocArena`].
#[repr(C)]
#[derive(Debug)]
pub struct MGAMallocNode {
    pub size: u64,
    pub pos: u64,
    pub data: *mut u8,
}

#[repr(C)]
#[derive(Debug)]
pub struct MGAReserveBackend {
    pub commit_pos: u64,
}

#[repr(C)]
pub struct MGAError {
    pub code: MGAErrorCode,
    pub msg: *mut c_char,
}

#[repr(C)]
#[derive(Debug)]
pub enum MGAErrorCode {
    None = 0,
    InitFailed,
    MallocFailed,
    OutOfNodes,
    CommitFailed,
    OutOfMemory,
}

pub type MGAErrorCallback = Option<unsafe extern "C" fn(code: MGAErrorCode, msg: *mut c_char)>;

/// A temporary arena, see [`mga_temp_begin`].
#[repr(C)]
#[derive(Debug)]
pub struct MGATemp {
    pub arena: *mut MGArena,
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
