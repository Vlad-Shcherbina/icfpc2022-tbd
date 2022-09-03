use std::fmt::Write;
use postgres::Client;

use crate::basic::*;
use crate::invocation::{record_this_invocation, Status};
use crate::util::DateTime;

pub fn upload_solution(
    tx: &mut postgres::Transaction,
    problem_id: i32,
    moves: &[Move],
    solver_name: &str,
    solver_args: &serde_json::Value,
    invocation_id: i32,
) -> i32 {
    let problem = Problem::load(problem_id);

    let mut painter = PainterState::new(&problem);
    let mut solution_text = String::new();
    for m in moves {
        painter.apply_move(m);
        writeln!(solution_text, "{}", m).unwrap();
    }

    let moves_cost = painter.cost;

    let img = painter.render();
    let dist = image_distance(&img, &problem.target).round() as i64;

    let row = tx.query_one("
    INSERT INTO solutions(problem_id, data, moves_cost, image_distance, solver, solver_args, invocation_id, timestamp)
    VALUES ($1, $2, $3, $4, $5, $6, $7, NOW())
    RETURNING id
    ", &[&problem_id, &solution_text, &moves_cost, &dist, &solver_name, &solver_args, &invocation_id]).unwrap();
    let id: i32 = row.get(0);

    eprintln!("Uploading solution for problem {}, cost={}, dist={}, total={} (solution/{})",
        problem_id, moves_cost, dist, moves_cost + dist, id);

    id
}

pub fn get_solution(client: &mut Client, solution_id: i32) -> SolutionRow {
    let query = "
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
    WHERE id = $1";
    let r = client.query_one(query, &[&solution_id]).unwrap();
    let moves_cost = r.get("moves_cost");
    let image_distance = r.get("image_distance");
    // TODO: there's a bug here
    // thread 'main' panicked at 'error retrieving column solver_args: error deserializing column 6: invalid type: map, expected a sequence at line 1 column 0', /home/sweater/.cargo/registry/src/github.com-1ecc6299db9ec823/tokio-postgres-0.7.7/src/row.rs:151:25
    // let solver_args: Json<Option<Vec<String>>> = r.get("solver_args");
    SolutionRow {
        id: r.get("id"),
        problem_id: r.get("problem_id"),
        data: r.get("data"),
        moves_cost,
        image_distance,
        score: (moves_cost + image_distance),
        solver_name: r.get("solver"),
        invocation_id: r.get("invocation_id"),
        timestamp: r.get("timestamp")
    }
}

pub fn best_solution(client: &mut Client, problem_id: i32) -> SolutionRow {
    let query = "
    SELECT
        id,
        problem_id,
        data,
        moves_cost,
        image_distance,
        solver,
        invocation_id,
        timestamp
    FROM solutions s1
        WHERE (image_distance + moves_cost) = (
            SELECT MIN ( s2.image_distance + s2.moves_cost )
            FROM solutions s2
            WHERE s1.problem_id = s2.problem_id
        ) AND ($1 = s1.problem_id)
    ";
    let rs = client.query(query, &[&problem_id]).unwrap();
    let r = rs.first().unwrap();
    let moves_cost = r.get("moves_cost");
    let image_distance = r.get("image_distance");
    // eprintln!("here");
    // TODO: there's a bug here
    // thread 'main' panicked at 'error retrieving column solver_args: error deserializing column 6: invalid type: map, expected a sequence at line 1 column 0', /home/sweater/.cargo/registry/src/github.com-1ecc6299db9ec823/tokio-postgres-0.7.7/src/row.rs:151:25
    // let solver_args: Json<Option<Vec<String>>> = r.get("solver_args");
    // eprintln!("there");
    SolutionRow {
        id: r.get("id"),
        problem_id: r.get("problem_id"),
        data: r.get("data"),
        moves_cost,
        image_distance,
        score: (moves_cost + image_distance),
        solver_name: r.get("solver"),
        invocation_id: r.get("invocation_id"),
        timestamp: r.get("timestamp")
    }
}

pub struct SolutionRow {
    pub id: i32,
    pub problem_id: i32,
    pub data: String,
    pub moves_cost: i64,
    pub image_distance: i64,
    pub score: i64,
    pub solver_name: String,
    pub invocation_id: i32,
    pub timestamp: DateTime
}

crate::entry_point!("upload_solution", upload_solution_ep);
fn upload_solution_ep() {
    let mut pargs = pico_args::Arguments::from_vec(std::env::args_os().skip(2).collect());
    let problem_id: i32 = pargs.value_from_str("--problem").unwrap();
    let solution_path: String = pargs.value_from_str("--solution").unwrap();
    let solver_name: String = pargs.value_from_str("--solver").unwrap();
    let dry_run = pargs.contains("--dry-run");
    let rest = pargs.finish();
    assert!(rest.is_empty(), "unrecognized arguments {:?}", rest);

    let solution_text = std::fs::read_to_string(solution_path).unwrap();
    let moves = Move::parse_many(&solution_text);

    let mut client = crate::db::create_client();
    let mut tx = client.transaction().unwrap();
    let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
    upload_solution(&mut tx, problem_id, &moves, &solver_name, &serde_json::Value::Null, incovation_id);
    if dry_run {
        eprintln!("But not really, because it was a --dry-run!");
    } else {
        tx.commit().unwrap();
    }
}
