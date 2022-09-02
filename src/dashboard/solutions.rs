// use std::time::SystemTime;
use askama::Template;
use super::dev_server::{Request, ResponseBuilder, HandlerResult};
// use crate::invocation::Invocation;
// use crate::solution::Solution;
// use crate::util::DateTime;

pub fn handler(_state: &std::sync::Mutex<super::State>, req: Request, resp: ResponseBuilder) -> HandlerResult {
    if req.path.is_empty() {
        //let client = &mut state.lock().unwrap().client;
        let s = SolutionsTemplate {}.render().unwrap();
        return resp.code("200 OK").body(s);
    }

    resp.code("404 Not Found").body("not found")
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}sol/{% endblock %}
{% block body %}
<h1>Here be dragons</h1>
{% endblock %}
"#)]
struct SolutionsTemplate {}
