# Install Rust

You need relatively recent Rust nightly toolchain.

On any Linux:

1. ```
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs > rustup.sh
   sh rustup.sh --default-toolchain nightly --profile minimal -y
   rm rustup.sh
   ```
2. ```
   sudo apt install -y lld
   ```

On Windows:

1. Download `rustup.exe` from https://rustup.rs/
2. Run it.
3. Select "toolchain: nightly", "profile: minimal", leave everything else as is.
4. Select "Install".
5. You also need to install Visual Studio or Visual Studio Build Tools (C++).

To check it's installed:

```
$ cargo --version
cargo 1.65.0-nightly (9809f8ff3 2022-08-16)
$ rustc --version
rustc 1.65.0-nightly (878aef79d 2022-08-20)
```

# Check that it all works

```
cargo test
cargo run hello
```

# IDE

You can use whatever. VSCode with `rust-analyzer` extension is a safe choice.
