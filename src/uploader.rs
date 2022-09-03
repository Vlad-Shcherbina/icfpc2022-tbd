use std::fmt::Write;
use crate::basic::*;
use crate::image::Image;
use crate::util::project_path;
use crate::invocation::{record_this_invocation, Status};

pub fn upload_solution(
    tx: &mut postgres::Transaction,
    problem_id: i32,
    moves: &[Move],
    solver_name: &str,
    solver_args: &serde_json::Value,
    invocation_id: i32,
) -> i32 {
    let target = Image::load(&project_path(format!("data/problems/{}.png", problem_id)));

    let mut painter = PainterState::new(target.width, target.height);
    let mut solution_text = String::new();
    for m in moves {
        painter.apply_move(m);
        writeln!(solution_text, "{}", m).unwrap();
    }

    let moves_cost = painter.cost;

    let img = painter.render();
    let dist = image_distance(&img, &target).round() as i64;

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
