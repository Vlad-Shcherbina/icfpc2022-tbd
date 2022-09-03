Local Web UI could be useful for all kinds of user interfaces.
Maybe we want to show large tables, maybe draw some HTML5 canvas stuff, maybe there will be some interactivity.

### How to run

1. Install TypeScript:
    ```
    sudo apt install nodejs npm
    npm install -g typescript@latest
    ```

2. Start TypeScript compiler in watch mode and leave it to run (for example in a separate terminal):
    ```
    tsc --watch
    ```

3. Start the dashboard server:
    ```
    cargo run dashboard
    ```
    To automatically recompile and restart server on any change, you can instead run
    ```
    cargo install cargo-watch
    cargo watch -x "run dashboard"
    ```

4. Navigate to http://localhost:8080/

On Windows, you can't modify a running executable.
It's annoying if you have a long-running dashboard in the background
and working on some other entry point that you want to run from time to time.
As a workaround, you can run the dashboard with `--release`, it will be a different executable.
