# Clippy

Clippy is a Rust linter.

Some of its suggestions catch bugs, some teach useful idioms, and some are wrong.

It's hard to spot helpful suggestions among the wall of noise.
So please address or silence each Clippy warning that pops up.

## Installation

```
rustup component add clippy
```

## Usage

```
cargo clippy
```

## Silencing warnings

The name of the warning is included in the warning message, e.g. `clippy::bytes_nth`.
If you disagree with the suggestion,
you can silence it by putting `#[allow(clippy::bytes_nth)]` before the function definition.
Or put ```#![allow(clippy::bytes_nth)]``` at the top of the file.
