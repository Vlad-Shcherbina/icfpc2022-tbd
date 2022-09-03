// use std::time::SystemTime;
use askama::Template;
use postgres::{types::Json};
use crate::{util::DateTime, invocation::Invocation};

use super::dev_server::{Request, ResponseBuilder, HandlerResult};
// use crate::invocation::Invocation;
// use crate::solution::Solution;
// use crate::util::DateTime;

pub fn handler(state: &std::sync::Mutex<super::State>, req: Request, resp: ResponseBuilder) -> HandlerResult {
    if req.path.is_empty() {
        let client = &mut state.lock().unwrap().client;
        let best_query = "
        SELECT
            id,
            problem_id,
            data,
            moves_cost,
            image_distance,
            solver,
            solver_args,
            invocation_id,
            timestamp
        FROM solutions s1
        WHERE (image_distance + moves_cost) = (
            SELECT MIN ( s2.image_distance + s2.moves_cost )
            FROM solutions s2
            WHERE s1.problem_id = s2.problem_id
        ) AND ($1 = -1 OR $1 = s1.problem_id)
        ORDER BY problem_id DESC
        ";

        let all_query = "
        SELECT
            id,
            problem_id,
            data,
            moves_cost,
            image_distance,
            solver,
            solver_args,
            invocation_id,
            timestamp
        FROM solutions
        WHERE $1 = -1 OR $1 = problem_id
        ";

        let archive = req.query_args.remove("archive");
        let problem_id: i32 = req.query_args.remove("problem_id").map_or(-1, |s| s.parse().unwrap());

        let archive = match archive {
            Some(x) => x == "true",
            None => false
        };
        let query = if archive {
            all_query
        } else {
            best_query
        };

        eprintln!("RUNNING: {}", query);

        let opts = SolutionsOpts { archive };

        let raw_rows = client.query(query, &[&problem_id]).unwrap();
        let rows = raw_rows.into_iter().map(|row| {
            let id: i32 = row.get("id");
            let problem_id: i32 = row.get("problem_id");
            // let data: String = row.get("data");
            let moves_cost: i64 = row.get("moves_cost");
            let image_distance: i64 = row.get("image_distance");
            let score: i64 = moves_cost + image_distance;
            let solver_name: String = row.get("solver");
            let solver_args: Json<Option<Vec<String>>> = row.get("solver_args");
            let invocation_id: i32 = row.get("invocation_id");
            let timestamp: DateTime = row.get("timestamp");
            SolutionRow { id, problem_id, /* data, */ moves_cost, image_distance, score, solver_name, solver_args: solver_args.0, invocation_id, timestamp }
        }).collect();
        let s = SolutionsTemplate {opts, rows}.render().unwrap();
        return resp.code("200 OK").body(s);
    }

    if let Ok(id) = req.path.parse::<i32>() {
        let client = &mut state.lock().unwrap().client;
        let raw_row = client.query_one("SELECT problem_id, data, moves_cost, image_distance, invocation_id FROM solutions WHERE id = $1", &[&id]).unwrap();
        let problem_id: i32 = raw_row.get("problem_id");
        let moves_cost: i64 = raw_row.get("moves_cost");
        let image_distance: i64 = raw_row.get("image_distance");
        let invocation_id: i32 = raw_row.get("invocation_id");

        let inv_row = client.query_one("SELECT data FROM invocations WHERE id = $1", &[&invocation_id]).unwrap();
        let postgres::types::Json(inv_data) = inv_row.get("data");

        let data: String = raw_row.get("data");

        let s = SolutionTemplate { id, problem_id, moves_cost, image_distance, data, invocation_id, inv_data }.render().unwrap();
        return resp.code("200 OK").body(s);
    }

    resp.code("404 Not Found").body("not found")
}

struct SolutionRow {
    id: i32,
    problem_id: i32,
    //data: String,
    moves_cost: i64,
    image_distance: i64,
    score: i64,
    solver_name: String,
    solver_args: Option<Vec<String>>,
    invocation_id: i32,
    timestamp: DateTime
}

struct SolutionsOpts {
    archive: bool,
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}sol/{% endblock %}
{% block body %}

<a href="?">best</a> |
<a href="?archive={{ !opts.archive }}">archive (all)</a>
<hr />
<table>
<thead>
<tr>
    <th>timestamp</th>
    <th>pr.</th>
    <th>score</th>
    <th></th>
    <th>image dist.</th>
    <th></th>
    <th>moves cost</th>
    <th></th>
    <th>solver name</th>
    <th>solver args</th>
    <th>inv.<th>
</tr>
</thead>
{% for row in rows %}
    <tr>
        <td>{{ row.timestamp.format("%d %H:%M:%S").to_string() }}</td>
        <!-- {{ row.id }} -->
        <td><a href="/solution/?problem_id={{ row.problem_id }}&archive=true">#{{ row.problem_id }}</a></td>
        <td style="text-align: right">{{ row.score }}</td>
        <td> = </td>
        <td style="text-align: right">{{ row.image_distance }}</td>
        <td>+</td>
        <td>{{ row.moves_cost }}</td>
        <td><a href="/solution/{{ row.id }}">sol/{{ row.id }}</a></td>
        <td>{{ row.solver_name }}</td>
        <td>[ {% for arg in row.solver_args %} arg {% endfor %} ]</td>
        <td><a href="/invocation/{{ row.invocation_id }}">inv/{{ row.invocation_id }}</a></td>
    </tr>
{% endfor %}
</table>
{% endblock %}
"#)]
struct SolutionsTemplate {
    opts: SolutionsOpts,
    rows: Vec<SolutionRow>,
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}sol/{{ id }}{% endblock %}
{% block body %}
<p>{{ inv_data|render_invocation_ref(invocation_id)|safe }}</p>
<p>Score: {{ moves_cost + image_distance }} = {{ image_distance }} + {{ moves_cost }}</p>
<p>Target: <img src="/data/problems/{{ problem_id }}.png"></img></p>
<pre>{{ data }}</pre>
{% endblock %}
"#)]
struct SolutionTemplate {
    id: i32,
    problem_id: i32,
    moves_cost: i64,
    image_distance: i64,
    data: String,
    invocation_id: i32,
    inv_data: Invocation,
}

mod filters {
    pub use crate::dashboard::invocations::filters::render_invocation_ref;
}