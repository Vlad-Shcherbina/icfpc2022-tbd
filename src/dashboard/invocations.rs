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

    if let Ok(id) = req.path.parse::<i32>() {
        let client = &mut state.lock().unwrap().client;
        let raw_row = client.query_one("
        SELECT id, status, start_time, update_time, data
        FROM invocations
        WHERE id = $1", &[&id]).unwrap();
        let status: String = raw_row.get("status");
        let start_time: SystemTime = raw_row.get("start_time");
        let start_time = DateTime::from(start_time);
        let update_time: SystemTime = raw_row.get("update_time");
        let update_time = DateTime::from(update_time);
        let postgres::types::Json(data) = raw_row.get("data");
        let row = InvocationRow { id, status, start_time, update_time, data };

        let formatted_data = serde_json::to_string_pretty(&row.data).unwrap();
        let s = InvocationTemplate { row, formatted_data }.render().unwrap();
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
        <td>{{ row.start_time.format("%d %H:%M:%S").to_string() }}</td>
        <td>
            {% if row.status != "RUN" %}
                {{ row.update_time.format("%d %H:%M:%S").to_string() }}
            {% endif %}
        </td>
        <td>{{ row.status }}</td>
        <td>{{ row.data|render_invocation_ref(row.id)|safe }}</td>
    </tr>
{% endfor %}
</table>
{% endblock %}
"#)]
struct InvocationsTemplate {
    rows: Vec<InvocationRow>,
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}inv/{{row.id}}{% endblock %}
{% block body %}
<p>{{ row.data|render_invocation_ref(row.id)|safe }}</p>
<p>Started: {{ row.start_time.format("%d %H:%M:%S").to_string() }}</p>
<p>Updated: {{ row.update_time.format("%d %H:%M:%S").to_string() }}</p>
<p>Status: {{ row.status }}</p>
<pre>{{ formatted_data }}</pre>

<hr>
<p>
    Stuff produced by this invocation should be listed here...
</p>
{% endblock %}
"#)]
struct InvocationTemplate {
    row: InvocationRow,
    formatted_data: String,
}

struct InvocationRow {
    id: i32,
    status: String,
    start_time: DateTime,
    update_time: DateTime,
    data: Invocation,
}

mod filters {
    use super::*;

    pub fn render_invocation_ref(inv: &Invocation, id: &i32) -> askama::Result<String> {
        InvocationRefTemplate { id: *id, inv }.render()
    }

    #[derive(Template)]
    #[template(ext = "html", source = r#"
    <a href="/invocation/{{ id }}">inv/{{ id }}</a>
    <span style="font-family: monospace">
    {% for arg in inv.argv[1..] %}
      {{ arg }}
    {% endfor %}
    </span>
    {{ inv.user }}@{{ inv.machine }}
    <a href="https://github.com/Vlad-Shcherbina/icfpc2022-tbd/commit/{{ inv.version.commit }}">{{ inv.version.commit[..8] }}</a>
    #{{ inv.version.commit_number }}
    {% if !inv.version.diff_stat.is_empty() %}
        <span title="{{ inv.version.diff_stat }}">dirty</span>
    {% endif %}
    "#)]
    struct InvocationRefTemplate<'a> {
        id: i32,
        inv: &'a Invocation,
    }
}
