#ifndef ROFI_RUST_H
#define ROFI_RUST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Version of the Rust component.
 *
 * The returned string is static and NUL-terminated. It must not be freed by
 * the caller.
 *
 * This function is implemented in `rust/lib.rs` and is linked into the rofi
 * binary when Rust support is enabled.
 */
const char *rofi_rust_version(void);

#ifdef __cplusplus
}
#endif

#endif /* ROFI_RUST_H */
