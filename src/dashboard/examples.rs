use askama::Template;
use super::dev_server::{Request, ResponseBuilder, HandlerResult};

pub fn handler(req: Request, resp: ResponseBuilder) -> HandlerResult {
    if req.path.is_empty() {
        let s = ExamplesTemplate {}.render().unwrap();
        return resp.code("200 OK").body(s);
    }
    if let Some(name) = req.path.strip_prefix("hello/") {
        let s = HelloTemplate { name }.render().unwrap();
        return resp.code("200 OK").body(s);
    }
    resp.code("400 Not Found").body("not found")
}

#[derive(Template)]
#[template(path = "examples.html")]
struct ExamplesTemplate {
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}hello{% endblock %}
{% block body %}
Hello, <b>{{ name }}</b>!
{% endblock %}
"#)]
struct HelloTemplate<'a> {
    name: &'a str,
}
