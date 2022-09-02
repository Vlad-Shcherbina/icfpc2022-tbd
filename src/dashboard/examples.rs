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
    if req.path == "ajax" {
        let s = AjaxTemplate {}.render().unwrap();
        return resp.code("200 OK").body(s);
    }
    if req.path == "interpreter" {
        let s = InterpreterTemplate {}.render().unwrap();
        return resp.code("200 OK").body(s);
    }
    if req.path == "foobar" {
        assert_eq!(req.method, "POST");
        let request: FoobarRequest = serde_json::from_str(std::str::from_utf8(req.body).unwrap()).unwrap();
        let response = FoobarResponse {
            s: "*".repeat(request.x),
            y: 42,
        };
        let s = serde_json::to_string(&response).unwrap();
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

#[derive(Template)]
#[template(path = "ajax.html")]
struct AjaxTemplate {
}

#[derive(Template)]
#[template(path = "interpreter.html")]
struct InterpreterTemplate {
}

// keep in sync with types.ts
#[derive(serde::Deserialize)]
struct FoobarRequest {
    x: usize,
}

// keep in sync with types.ts
#[derive(serde::Serialize)]
struct FoobarResponse {
    s: String,
    y: i32,
}
