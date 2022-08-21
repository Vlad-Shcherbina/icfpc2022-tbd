mod dev_server;
mod static_files;

use crate::util::project_root;

crate::entry_point!("dashboard", dashboard);
fn dashboard() {
    let listener = std::net::TcpListener::bind("127.0.0.1:8000").unwrap();
    eprintln!("serving at http://127.0.0.1:8000 ...");
    dev_server::serve_forever(listener, |req, resp| {
        if req.path == "/" {
            return resp.code("200 OK").body("hello");
        }

        static_files::static_handler(&project_root(), req, resp)
    });
}
