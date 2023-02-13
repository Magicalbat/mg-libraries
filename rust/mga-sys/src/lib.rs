use std::{ffi::c_void, mem::ManuallyDrop};

#[link(name = "mg_arena")]
extern "C" {
    pub fn mga_create(desc: *const MGADesc) -> *mut MGArena;
    pub fn mga_destroy(arena: *mut MGArena);

    pub fn mga_push(arena: *mut MGArena, size: u64) -> *mut c_void;
    pub fn mga_push_zero(arena: *mut MGArena, size: u64) -> *mut c_void;

    pub fn mga_pop(arena: *mut MGArena, size: u64);
    pub fn mga_pop_to(arena: *mut MGArena, pos: u64);

    pub fn mga_temp_begin(arena: *mut MGArena) -> MGTempArena;
    pub fn mga_temp_end(temp: MGTempArena);
}

#[repr(C)]
pub struct MGArena {
    pub pos: u64,

    pub size: u64,
    pub block_size: u64,
    pub align: u32,

    pub _backend: MGArenaBackend,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MGADesc {
    pub max_size: u64,
    pub page_size: u32,
    pub pages_per_block: u32,
    pub align: u32,
}

#[repr(C)]
pub union MGArenaBackend {
    pub _malloc_arena: ManuallyDrop<MGAMallocArena>,
    pub _reserve_arena: ManuallyDrop<MGAReserveArena>,
}

#[repr(C)]
#[derive(Debug)]
pub struct MGAMallocArena {
    first: *mut MGAMallocNode,
    last: *mut MGAMallocNode,
    num_nodes: u32,
}

#[repr(C)]
#[derive(Debug)]
pub struct MGAMallocNode {
    pub next: *mut MGAMallocNode,
    pub pos: u64,
    pub data: *mut u8,
}

#[repr(C)]
#[derive(Debug)]
pub struct MGAReserveArena {
    commit_pos: u64,
}

#[repr(C)]
#[derive(Debug)]
pub struct MGTempArena {
    arena: *mut MGArena,
    pos: u64,
}

pub const fn mga_kb(x: u64) -> u64 {
    x * 1_000
}

pub const fn mga_mb(x: u64) -> u64 {
    x * 1_000_000
}

pub const fn mga_gb(x: u64) -> u64 {
    x * 1_000_000_000
}

pub const fn mga_kib(x: u64) -> u64 {
    x << 10
}

pub const fn mga_mib(x: u64) -> u64 {
    x << 20
}

pub const fn mga_gib(x: u64) -> u64 {
    x << 30
}
