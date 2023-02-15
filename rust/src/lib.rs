#[cfg(feature = "export-sys")]
pub use mga_sys as sys;

pub mod lowlevel;

pub type ArenaResult<T> = Result<T, mga_sys::MGAError>;

pub use mga_sys::{mga_gib as gib_to_bytes, mga_kib as kib_to_bytes, mga_mib as mib_to_bytes};
