# Entry points

This Rust project is a single executable.
But you can have more than one "main" function.

You can annotate any function as like this:
```rust
crate::entry_point!("foobar", foobar);
fn foobar() {
    ...
}
```

And then run it with
```
cargo run foobar
```
