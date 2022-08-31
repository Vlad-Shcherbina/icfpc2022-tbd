use std::time::SystemTime;
use askama::Template;
use super::dev_server::{Request, ResponseBuilder, HandlerResult};
use crate::invocation::Invocation;
use crate::util::DateTime;

pub fn handler(state: &std::sync::Mutex<super::State>, req: Request, resp: ResponseBuilder) -> HandlerResult {
    if req.path.is_empty() {  // list all invocations
        let client = &mut state.lock().unwrap().client;

        let raw_rows = client.query("
        SELECT id, status, start_time, update_time, data FROM invocations
        ", &[]).unwrap();
        let mut rows: Vec<_> = raw_rows.into_iter().map(|row| {
            let id: i32 = row.get("id");
            let status: String = row.get("status");
            let start_time: SystemTime = row.get("start_time");
            let start_time = DateTime::from(start_time);
            let update_time: SystemTime = row.get("update_time");
            let update_time = DateTime::from(update_time);
            let postgres::types::Json(data) = row.get("data");
            InvocationRow { id, status, start_time, update_time, data }
        }).collect();
        rows.sort_by_key(|r| if r.status == "RUN" { (2, r.start_time) } else { (1, r.update_time) });
        rows.reverse();
        let s = InvocationsTemplate { rows }.render().unwrap();
        return resp.code("200 OK").body(s);
    }
    resp.code("400 Not Found").body("not found")
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}Invocations{% endblock %}
{% block body %}
<table>
{% for row in rows %}
    <tr>
        <td>{{ row.id }}</td>
        <td>{{ row.status }}</td>
        <td>{{ row.start_time.format("%d %H:%M:%S").to_string() }}</td>
        <td>
            {% if row.status != "RUN" %}
                {{ row.update_time.format("%d %H:%M:%S").to_string() }}
            {% endif %}
        </td>
        <td>
            {{ row.data.user }}@{{ row.data.machine }}
            <a href="https://github.com/Vlad-Shcherbina/icfpc2022-tbd/commit/{{ row.data.version.commit }}">{{ row.data.version.commit[..8] }}</a>
            #{{ row.data.version.commit_number }}
            {% if !row.data.version.diff_stat.is_empty() %}
                <span title="{{ row.data.version.diff_stat }}">dirty</span>
            {% endif %}
        </td>
    </tr>
{% endfor %}
</table>
{% endblock %}
"#)]
struct InvocationsTemplate {
    rows: Vec<InvocationRow>,
}

struct InvocationRow {
    id: i32,
    status: String,
    start_time: DateTime,
    update_time: DateTime,
    data: Invocation,
}
