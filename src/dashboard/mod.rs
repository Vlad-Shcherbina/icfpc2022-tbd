mod dev_server;
mod static_files;
mod examples;

use crate::util::project_root;
use dev_server::Request;

crate::entry_point!("dashboard", dashboard);
fn dashboard() {
    let listener = std::net::TcpListener::bind("127.0.0.1:8000").unwrap();
    eprintln!("serving at http://127.0.0.1:8000 ...");
    dev_server::serve_forever(listener, |req, resp| {
        if let Some(path) = req.path.strip_prefix("/example/") {
            return examples::handler(Request { path, ..req }, resp);
        }

        if req.path == "/" {
            return resp.code("303 See Other").header("Location", "/example/hello/world").body("");
        }

        static_files::static_handler(&project_root(), req, resp)
    });
}
