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
    let start = std::time::Instant::now();
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
    eprintln!("it took {:?}", start.elapsed());
    eprintln!("{}", crate::stats::STATS.render());
}

fn solve(problem: &Problem) -> Vec<Move> {
    let _t = crate::stats_timer!("solve").time_it();
    let mut painter = PainterState::new(problem);
    let mut all_moves = vec![];

    let (mut block_id, moves) = seg_util::merge_all(&mut painter);
    all_moves.extend(moves);

    let mut ys = vec![10, 70, 110, 250, 350, 390];
    let mut xss = vec![];
    for _ in 1..ys.len() {
        xss.push(vec![10, 70, 110, 250, 350, 390]);
    }

    loop {
        let (new_block, moves) = seg_util::isolate_rect(&mut painter, block_id, Shape {
            x1: 0,
            y1: ys[0],
            x2: problem.target.width,
            y2: *ys.last().unwrap(),
        });
        all_moves.extend(moves);
        block_id = new_block;

        let y1;
        let y2;
        let mut xs;

        let y_min = ys[0];
        let y_max = *ys.last().unwrap();

        if ys[1] - ys[0] < ys[ys.len() - 1] - ys[ys.len() - 2] {
            y1 = ys[0];
            y2 = ys[1];
            ys.remove(0);
            xs = xss.remove(0);
        } else {
            y1 = ys[ys.len() - 2];
            y2 = ys[ys.len() - 1];
            ys.pop().unwrap();
            xs = xss.pop().unwrap();
        }

        loop {
            let (new_block, moves) = seg_util::isolate_rect(&mut painter, block_id, Shape {
                x1: xs[0],
                y1: y_min,
                x2: *xs.last().unwrap(),
                y2: y_max,
            });
            all_moves.extend(moves);
            block_id = new_block;

            let x1;
            let x2;
            if xs[1] - xs[0] < xs[xs.len() - 1] - xs[xs.len() - 2] {
                x1 = xs[0];
                x2 = xs[1];
                xs.remove(0);
            } else {
                x1 = xs[xs.len() - 2];
                x2 = xs[xs.len() - 1];
                xs.pop().unwrap();
            }

            let color = crate::color_util::optimal_color_for_block(
                &problem.target, &Shape { x1, y1, x2, y2 });
            let m = ColorMove { block_id: block_id.clone(), color };
            painter.apply_move(&m);
            all_moves.push(m);
            if xs.len() == 1 {
                break;
            }
        }

        if ys.len() == 1 {
            break;
        }

        let (new_block, moves) = seg_util::merge_all_inside_bb(&mut painter, Shape {
            x1: 0,
            y1: y_min,
            x2: problem.target.width,
            y2: y_max,
        });
        all_moves.extend(moves);
        block_id = new_block;
    }

    all_moves
}
