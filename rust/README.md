# Rust components

The `rust/` directory contains Rust code that is compiled into the same
`rofi` binary as the C sources. This is not a separate executable or a shared
library; Meson builds a Rust `staticlib` and links it into the `rofi` target.

## Requirements

- `rustc` with Rust 2018 edition support (Rust 1.31 or newer).
- Meson 0.57 or newer. Rofi currently requires Meson 0.59 or newer.

Meson's native Rust support is used, so Cargo is not required. Rust support is
controlled by the `rust` feature option:

```sh
# auto-detect rustc (default)
meson setup build

# require Rust
meson setup build -Drust=enabled

# do not build Rust components
meson setup build -Drust=disabled
```

## Layout

- `lib.rs` is the crate root and is listed as a source in `meson.build`.
- Add extra modules as `rust/foo.rs`, declare `mod foo;` in `lib.rs`, and add
  `rust/foo.rs` to the `files()` list for the `rofi_rust` target so Meson
  tracks it for incremental builds.
- Public C entry points are declared in `include/rofi-rust.h`.

## C ABI rules

Exported functions must use the C ABI and a stable symbol name:

```rust
#[no_mangle]
pub extern "C" fn rofi_example(value: u32) -> u32 {
    value + 1
}
```

Every exported function needs a matching declaration in
`include/rofi-rust.h`, guarded by `extern "C"` for C++ users.

Rust panics must never unwind across the C boundary. Keep exported functions
panic-free, or catch unwinds with `std::panic::catch_unwind` before returning.
The staticlib is built with Meson's default panic strategy; if you add code
that can panic, either handle it locally or configure a panic-abort strategy
for the target.

## External crates

Meson's native Rust integration compiles one crate with `rustc`; it does not
resolve `Cargo.toml` dependencies or download crates. If a component needs
external crates, vendor the dependency as a Meson subproject (or expose it as
an rlib) and add it to the Rust target's dependencies. Alternatively, replace
the `static_library()` rule in `meson.build` with a Cargo `custom_target()`
that produces a static archive; the rest of the C/Rust linking setup remains
the same.
