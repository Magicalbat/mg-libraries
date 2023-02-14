#[cfg(feature = "export-sys")]
pub use mga_sys as sys;

pub mod lowlevel;

pub use mga_sys::{
    mga_gb as gb_to_bytes, mga_gib as gib_to_bytes, mga_kb as kb_to_bytes, mga_kib as kib_to_bytes,
    mga_mb as mb_to_bytes, mga_mib as mib_to_bytes,
};
