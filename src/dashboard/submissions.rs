use askama::Template;

use super::dev_server::{ResponseBuilder, Request, HandlerResult};

pub fn handler(
  _state: &std::sync::Mutex<super::State>,
  req: Request,
  resp: ResponseBuilder
) -> HandlerResult {
  if req.path.is_empty() {
    let s = SubmissionsTemplate {}.render().unwrap();
    return resp.code("200 OK").body(s);
  }
  resp.code("400 Not Found").body("not found")
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}sol/{% endblock %}
{% block body %}
<h1>Here be dragons</h1>
{% endblock %}
"#)]
struct SubmissionsTemplate {}