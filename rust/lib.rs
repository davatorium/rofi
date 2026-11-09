//! Rust components for rofi.
//!
//! This file is the root of a Rust `staticlib` crate that is linked directly
//! into the `rofi` executable. Only functions that are declared
//! `#[no_mangle] pub extern "C"` are visible to the C code; their declarations
//! live in `include/rofi-rust.h`.
//!
//! Rust code in this crate must not unwind across the C ABI. Keep exported
//! functions panic-free, or catch panics before returning (see
//! `rust/README.md`).

use std::os::raw::c_char;

/// Version reported by the Rust component.
///
/// Stored as bytes with an explicit trailing NUL so it can be returned
/// directly as a C string without allocating.
static RUST_VERSION: &[u8] = b"0.1.0\0";

/// Return the version of the Rust component.
///
/// The returned pointer is static, NUL-terminated and remains valid for the
/// lifetime of the process. The caller must not free it.
#[no_mangle]
pub extern "C" fn rofi_rust_version() -> *const c_char {
    RUST_VERSION.as_ptr() as *const c_char
}
