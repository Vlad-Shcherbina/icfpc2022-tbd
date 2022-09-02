use std::path::{Path, PathBuf};
use crate::dashboard::dev_server::{Request, ResponseBuilder, HandlerResult};

fn sanitize_url_to_path(url: &str) -> Result<&Path, ()> {
    let url = url.strip_prefix('/').unwrap();
    if url.contains('\\') || url.contains(':') {
        return Err(());
    }
    for part in url.split('/') {
        if part.is_empty() || part.starts_with('.') {
            return Err(());
        }
    }
    Ok(Path::new(url))
}

pub fn static_handler(root_path: &std::path::Path, req: Request, resp: ResponseBuilder) -> HandlerResult {
    assert_eq!(req.method, "GET");
    let Ok(path) = sanitize_url_to_path(req.path) else {
        return resp.code("400 Not Found").body("not found");
    };
    let path: PathBuf = [root_path, path].iter().collect();
    let Ok(data) = std::fs::read(&path) else {
        return resp.code("400 Not Found").body("not found");
    };
    let mime = match path.extension().and_then(|s| s.to_str()).unwrap_or_default() {
        "html" => "text/html",
        "css" => "text/css",
        "js" => "application/javascript",
        "map" => "text/plain",
        "ts" => "text/plain",
        "png" => "image/png",
        _ => todo!("{}", path.to_string_lossy()),
    };

    resp.code("200 OK")
        .header("Content-Type", mime)
        .body(data)
}
