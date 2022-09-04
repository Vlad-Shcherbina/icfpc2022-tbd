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

crate::entry_point!("brick_solver", brick_solver);
fn brick_solver() {
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
        upload_solution(&mut tx, problem_id, &moves, "brick", &serde_json::Value::Null, incovation_id);
        if dry_run {
            eprintln!("But not really, because it was a --dry-run!");
            break;  // because record_this_invocation() doesn't play well with reverted transactions
        } else {
            tx.commit().unwrap();
        }
    }
}

fn solve(_p: &Problem) -> Vec<Move> {
    vec![
        ColorMove { block_id: BlockId::root(0), color: Color([123, 45, 67, 255]) },
    ]
}
