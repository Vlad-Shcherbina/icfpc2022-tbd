// use std::time::SystemTime;
use askama::Template;
use postgres::{types::Json};
use crate::util::DateTime;

use super::dev_server::{Request, ResponseBuilder, HandlerResult};
// use crate::invocation::Invocation;
// use crate::solution::Solution;
// use crate::util::DateTime;

pub fn handler(state: &std::sync::Mutex<super::State>, req: Request, resp: ResponseBuilder) -> HandlerResult {
    if req.path.is_empty() {
        let client = &mut state.lock().unwrap().client;

        // let best_query = "
        // SELECT
        //     problem_id,
        //     id,
        //     data,
        //     moves_cost,
        //     image_distance,
        //     (moves_cost + image_distance) AS score,
        //     MIN(moves_cost + image_distance),
        //     solver,
        //     solver_args,
        //     invocation_id,
        //     timestamp
        // FROM solutions
        // GROUP BY problem_id
        // ORDER BY score DESC
        // ";

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
        )
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
        ";

        let mut query = best_query;

        let archive = req.query_args.remove("archive");

        let archive = match archive {
            Some(x) => x == "true",
            None => false
        };
        if archive {
            query = all_query;
        }

        eprintln!("RUNNING: {}", query);

        let opts = SolutionsOpts { archive };

        let raw_rows = client.query(query, &[]).unwrap();
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
<a href="?archive={{ !opts.archive }}">archive</a>
<hr />
<table>
{% for row in rows %}
    <tr>
        <td>{{ row.timestamp.format("%d %H:%M:%S").to_string() }}</td>
        <!-- {{ row.id }} -->
        <td>#{{ row.problem_id }}</td>
        <td style="text-align: right">{{ row.score }}</td>
        <td> = </td>
        <td style="text-align: right">{{ row.image_distance }}</td>
        <td>+</td>
        <td>{{ row.moves_cost }}</td>
        <td>{{ row.solver_name }}</td>
        <td>[ {% for arg in row.solver_args %} arg {% endfor %} ]</td>
        <td>{{ row.invocation_id }}</td>
    </tr>
{% endfor %}
</table>
{% endblock %}
"#)]
struct SolutionsTemplate {
    opts: SolutionsOpts,
    rows: Vec<SolutionRow>,
}
