#[cfg(feature = "export-sys")]
pub mod sys {
    pub use mga_sys::*;
}

#[cfg(not(feature = "export-sys"))]
mod sys {
    pub use mga_sys::*;
}

mod arena;

pub use crate::arena::*;
