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

crate::entry_point!("raster_solver", raster_solver);
fn raster_solver() {
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
        upload_solution(&mut tx, problem_id, &moves, "raster", &serde_json::Value::Null, incovation_id);
        if dry_run {
            eprintln!("But not really, because it was a --dry-run!");
            break;  // because record_this_invocation() doesn't play well with reverted transactions
        } else {
            tx.commit().unwrap();
        }
    }
}

fn get_target_pixel(x: i32, y: i32, problem: &Problem) -> Color {
    // HACK: a quick way to turn the image black and white
    // (#4 and #5 have some gray in them that screws things up)
    let color = problem.target.get_pixel(x, y);
    let r = color.0[0] as i32;
    let g = color.0[1] as i32;
    let b = color.0[2] as i32;
    if r*r + g*g + b*b < 3 * 128_i32.pow(2) {
        Color([0, 0, 0, 255])
    } else {
        Color([255, 255, 255, 255])
    }
    
}

fn line_needs_paint(y: i32, problem: &Problem, painter: &PainterState) -> bool {
    let img = painter.render();
    for x in 0..problem.width {
        if img.get_pixel(x, y) != get_target_pixel(x, y, problem) {
            return true;
        }
    }
    false
}

fn paint_line(y: i32, problem: &Problem, painter: &mut PainterState, block_id: &BlockId) -> (Vec<Move>, BlockId) {
    let mut moves = vec![];
    let mut x = 0;
    let mut big_block_id = block_id.clone();
    loop {
        let img = painter.render();
        while x < problem.width && img.get_pixel(x, y) == get_target_pixel(x, y, problem) {
            x += 1;
        }
        if x == problem.width {
            break;
        }
        let target_color = get_target_pixel(x, y, problem);
        if x > 0 {
            let m = Move::LCut { block_id: big_block_id.clone(), orientation: Orientation::Vertical, line_number: x };
            painter.apply_move(&m);
            moves.push(m);

            let m = Move::ColorMove { block_id: big_block_id.child(1), color: target_color };
            painter.apply_move(&m);
            moves.push(m);

            let m = Move::Merge {block_id1: big_block_id.child(0), block_id2: big_block_id.child(1)};
            let ApplyMoveResult {cost: _, new_block_ids} = painter.apply_move(&m);
            moves.push(m);

            big_block_id = new_block_ids[0].clone();
        } else {
            // Just paint
            let m = Move::ColorMove { block_id: big_block_id.clone(), color: target_color };
            painter.apply_move(&m);
            moves.push(m);
        }
        x += 1;
    }
    (moves, big_block_id)
}

fn solve(problem: &Problem) -> Vec<Move> {
    let mut result = vec![];
    let mut painter = PainterState::new(problem);
    let mut big_block_id = BlockId::root(0);

    // For #4 we can improve by cutting off "2022" below: y in 70..
    for y in 0..problem.target.height {
        if !line_needs_paint(y, problem, &painter) {
            continue
        }

        if y > 0 {
            let m  = Move::LCut { block_id: big_block_id.clone(), orientation: Orientation::Horizontal, line_number: y };
            painter.apply_move(&m);
            result.push(m);
            big_block_id = big_block_id.child(1);
        }
        let (moves, new_big_block_id) = paint_line(y, problem, &mut painter, &big_block_id);
        big_block_id = new_big_block_id;
        result.extend(moves);
    }
    result
}
