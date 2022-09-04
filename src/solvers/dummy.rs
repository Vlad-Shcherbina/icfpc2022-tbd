// Feel free to copy it and do your own stuff.
// Don't call you copy anything like ExampleSolver, DumbSolver, SimpleSolver,
// HeuristicSolver, AStarSolver, or even <your idea>Solver!
// Even if you have one specific easy to describe idea behind your solver,
// over time it will evolve and diverge from this description.
// It's annoying to have GreedySolver that actualy does dynamic programming etc.
// Instead, use memorable and distinctive code names
// (preferably ones that are easy to pronounce too).
// Don't copy this comment when you copy the code.

#![allow(unused_imports)]

use crate::util::project_path;
use crate::basic::*;
use crate::image::Image;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;
use crate::color_util::*;
use crate::seg_util;

use crate::basic::Move::*;

#[derive(serde::Serialize)]
#[derive(Debug)]
struct SolverArgs {
    px: i32,
    py: i32,
    num_colors: usize,
}

crate::entry_point!("dummy_solver", dummy_solver);
fn dummy_solver() {
    let mut pargs = pico_args::Arguments::from_vec(std::env::args_os().skip(2).collect());
    let problems: String = pargs.value_from_str("--problem").unwrap();
    let dry_run = pargs.contains("--dry-run");
    let rest = pargs.finish();
    assert!(rest.is_empty(), "unrecognized arguments {:?}", rest);

    let problem_range = crate::util::parse_range(&problems);
    let mut client = crate::db::create_client();
    for problem_id in problem_range {
        eprintln!("*********** problem {} ***********", problem_id);
        let problem = Problem::load(problem_id);
        let moves = solve(&problem);
        let mut tx = client.transaction().unwrap();
        let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
        upload_solution(&mut tx, problem_id, &moves, "dummy", &serde_json::Value::Null, incovation_id);
        if dry_run {
            eprintln!("But not really, because it was a --dry-run!");
            break;  // because record_this_invocation() doesn't play well with reverted transactions
        } else {
            tx.commit().unwrap();
        }
    }
}

fn solve(problem: &Problem) -> Vec<Move> {
    let color = crate::color_util::optimal_color_for_block(
        &problem.target, &Shape { x1: 0, y1: 0, x2: problem.target.width, y2: problem.target.height });
    vec![
        Move::ColorMove {
            block_id: BlockId::root(0),
            color,
        },
    ]
}
