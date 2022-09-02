use std::fmt::Write;
use crate::basic::*;
use crate::image::Image;
use crate::util::project_path;

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

    let moves_cost = painter.cost as i64;

    let img = painter.render();
    let dist = image_distance(&img, &target).round() as i64;

    let row = tx.query_one("
    INSERT INTO solutions(problem_id, data, moves_cost, image_distance, solver, solver_args, invocation_id, timestamp)
    VALUES ($1, $2, $3, $4, $5, $6, $7, NOW())
    RETURNING id
    ", &[&problem_id, &solution_text, &moves_cost, &dist, &solver_name, &solver_args, &invocation_id]).unwrap();
    let id: i32 = row.get(0);

    eprintln!("Uploading solution for problem {}, cost={}, dist={}, total={} (solution/{})",
        problem_id, moves_cost, dist, moves_cost as i64 + dist, id);

    id
}