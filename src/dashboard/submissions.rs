use askama::Template;

use crate::{api::check_submission, util::DateTime};

use super::dev_server::{ResponseBuilder, Request, HandlerResult};

pub fn handler(
  state: &std::sync::Mutex<super::State>,
  _req: Request,
  resp: ResponseBuilder
) -> HandlerResult {
  let client = &mut state.lock().unwrap().client;
  let neither_good_nor_bad = "
  SELECT s.submission_id, g.cost, b.error
  FROM submissions s
  LEFT JOIN
    good_submissions g ON g.submission_id = s.submission_id
  LEFT JOIN
    bad_submissions b ON b.submission_id = s.submission_id
  WHERE
    g.cost IS NULL AND b.error IS NULL
  ";
  eprintln!("RUNNING: {}", neither_good_nor_bad);
  // Side-effect that checks pending submissions
  client.query(
    neither_good_nor_bad,
    &[]
  ).unwrap().into_iter().for_each(|row| {
    let submission_id: i32 = row.get("submission_id");
    eprintln!("Checking submission {}...", submission_id);
    check_submission(client, submission_id);
  });
  let good_query = "
  SELECT
    s.timestamp AS timestamp,
    s0.submission_id AS submission_id,
    o.solver AS solver,
    o.solver_args AS solver_args,
    s.solution_id AS solution_id,
    s.problem_id AS problem_id,
    (o.image_distance + o.moves_cost) AS our_cost,
    s0.cost AS their_cost,
    s0.file_url AS file_url
  FROM good_submissions s0
  LEFT JOIN submissions s ON s.submission_id = s0.submission_id
  LEFT JOIN solutions o ON o.id = s.solution_id
  ORDER BY timestamp DESC
  ";
  eprintln!("RUNNING: {}", good_query);
  let goods =
    client.query(good_query, &[]).unwrap().into_iter()
      .map(|row| {
        let timestamp: DateTime = row.get("timestamp");
        let submission_id = row.get::<&str, i32>("submission_id");
        let solver_name = row.get::<&str, String>("solver");
        let solver_args = row.get::<&str, serde_json::Value>("solver_args");
        let solution_id = row.get::<&str, i32>("solution_id");
        let problem_id = row.get::<&str, i32>("problem_id");
        let our_cost = row.get::<&str, i64>("our_cost");
        let their_cost = row.get::<&str, i64>("their_cost");
        // TODO: Here we can notice a discrepancy and report it somewhere somehow!
        let file_url = row.get::<&str, String>("file_url");
        GoodSubmission { submission_id, solution_id, problem_id,
                         our_cost, their_cost, file_url, solver_name,
                         solver_args: solver_args.to_string(),
                         timestamp }
      }).collect();
  // todo: add bads
  let s = SubmissionsTemplate {goods}.render().unwrap();
  resp.code("200 OK").body(s)
}

struct GoodSubmission {
  timestamp: DateTime,
  submission_id: i32,
  solver_name: String,
  solver_args: String,
  solution_id: i32,
  problem_id: i32,
  our_cost: i64,
  their_cost: i64,
  file_url: String,
}

#[derive(Template)]
#[template(ext = "html", source = r#"
{% extends "base.html" %}
{% block title %}sub/{% endblock %}
{% block body %}
<table>
<thead>
<tr>
  <th>timestamp</th>
  <th>submission</th>
  <th>pr.</th>
  <th>solver</th>
  <th>solver args.</th>
  <th>solution</th>
  <th>our cost</th>
  <th>reference cost</th>
</tr>
</thead>
<tbody>
{% for g in goods %}
  <tr>
    <td>{{ g.timestamp.format("%d %H:%M:%S").to_string() }}</td>
    <td>#{{ g.submission_id }} <small><a href="{{ g.file_url }}">(dump)</a></small></td>
    <td><a href="/solution/?archive=true&problem_id={{ g.problem_id }}">#{{ g.problem_id }}</a></td>
    <td>{{ g.solver_name }}</td>
    <td>{{ g.solver_args }}</td>
    <td><a href="/solution/{{ g.solution_id }}">sol/{{ g.solution_id }}</a></td>
    <td>{{ g.our_cost }}</td>
    <td>{{ g.their_cost }}</td>
  </tr>
{% endfor %}
</tbody>
</table>
{% endblock %}
"#)]
struct SubmissionsTemplate {
  goods: Vec<GoodSubmission>
}