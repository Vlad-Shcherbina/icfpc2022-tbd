mod dev_server;

crate::entry_point!("dashboard", dashboard);
fn dashboard() {
    let listener = std::net::TcpListener::bind("127.0.0.1:8000").unwrap();
    eprintln!("serving at http://127.0.0.1:8000 ...");
    crate::dashboard::dev_server::serve_forever(listener, |req, resp| {
        if req.path == "/" {
            return resp.code("200 OK").body("hello");
        }

        resp.code("400 Not Found").body("not found")
    });
}
