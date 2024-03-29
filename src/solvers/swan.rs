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

crate::entry_point!("swan_solver", swan_solver);
fn swan_solver() {
    let mut pargs = pico_args::Arguments::from_vec(std::env::args_os().skip(2).collect());
    let problems: String = pargs.value_from_str("--problem").unwrap();
    let dry_run = pargs.contains("--dry-run");
    let rest = pargs.finish();
    assert!(rest.is_empty(), "unrecognized arguments {:?}", rest);

    let problem_range = match problems.split_once("..") {
        Some((left, right)) => {
            let left: i32 = left.parse().unwrap();
            let right: i32 = right.parse().unwrap();
            left..=right
        }
        None => {
            let problem: i32 = problems.parse().unwrap();
            problem..=problem
        }
    };
    dbg!(&problem_range);


    let mut argss = vec![];
    for num_colors in 2..=5 {
        for px in [40, 50, 80, 100] {
            for py in [40, 50, 80, 100] {
                argss.push(SolverArgs { px, py, num_colors });
            }
        }
    }

    let start = std::time::Instant::now();
    for problem_id in problem_range {
        eprintln!("*********** problem {} ***********", problem_id);
        let problem = Problem::load(problem_id);

        let (args, (total_score, moves)) = argss.iter()
            .map(|args| (args, solve(args, &problem)))
            .min_by_key(|&(_args, (score, _))| score).unwrap();

        eprintln!("BEST:  {:?}: {}", args, total_score);

        let mut client = crate::db::create_client();
        let mut tx = client.transaction().unwrap();
        let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
        upload_solution(&mut tx, problem_id, &moves, "swan", &serde_json::to_value(args).unwrap(), incovation_id);
        if dry_run {
            eprintln!("But not really, because it was a --dry-run!");
        } else {
            tx.commit().unwrap();
        }
        eprintln!("{}", crate::stats::STATS.render());
    }
    eprintln!("it took {:?}", start.elapsed());
}

fn solve(args: &SolverArgs, problem: &Problem) -> (i64, Vec<Move>) {
    let _t = crate::stats_timer!("solve").time_it();

    let &SolverArgs {
        px, py, num_colors,
    } = args;

    let w = problem.target.width / px;
    let h = problem.target.height / py;

    let mut color_freqss = vec![];
    for i in 0..h {
        for j in 0..w {
            let color_freqs = color_freqs(&problem.target, &Shape {
                x1: j * px,
                y1: i * py,
                x2: (j + 1) * px,
                y2: (i + 1) * py,
            });
            color_freqss.push(color_freqs);
        }
    }

    let mut palette = k_means(&color_freqss, num_colors);
    palette.sort();

    let mut approx_target = Image::new(problem.target.width, problem.target.height, Color::default());

    let mut color_cnts = vec![0; palette.len()];

    let mut mini_target = Image::new(w, h, Color::default());
    for i in 0..h {
        for j in 0..w {
            let shape = Shape {
                x1: j * px,
                y1: i * py,
                x2: (j + 1) * px,
                y2: (i + 1) * py,
            };
            let color_freqs = color_freqs(&problem.target, &shape);
            let color_idx = best_from_palette(&color_freqs, &palette);
            color_cnts[color_idx] += 1;
            approx_target.fill_rect(shape, palette[color_idx]);
            mini_target.set_pixel(j, i, palette[color_idx]);
        }
    }
    let mut rects: Vec<(Shape, Color)> = crate::pack::packing2(&mini_target);
    for (shape, _color) in &mut rects {
        shape.x1 *= px;
        shape.y1 *= py;
        shape.x2 *= px;
        shape.y2 *= py;
    }

    let mut painter = PainterState::new(problem);
    let mut all_moves = vec![];

    // let mut root = painter.blocks.keys().next().unwrap().clone();
    eprintln!("merging...");
    let (mut root, moves) = seg_util::merge_all(&mut painter);
    eprintln!("done");
    all_moves.extend(moves);
    assert_eq!(painter.blocks.len(), 1);

    for (shape, color) in &rects {
        let (id, moves) = seg_util::isolate_rect(&mut painter, root, *shape);
        all_moves.extend(moves);
        let m = ColorMove { block_id: id, color: *color };
        painter.apply_move(&m);
        all_moves.push(m);

        let (new_root, moves) = seg_util::merge_all(&mut painter);
        all_moves.extend(moves);
        root = new_root;
    }

    let mut cur_img = painter.render();

    loop {
        let mut best_rank = 0;
        let mut best_pair = ((-1, -1), (-1, -1));
        for i1 in 0..h {
            for j1 in 0..w {
                if approx_target.get_pixel(j1 * px, i1 * py) == cur_img.get_pixel(j1 * px, i1 * py) {
                    continue;
                }
                for i2 in 0..h {
                    for j2 in 0..w {
                        if approx_target.get_pixel(j2 * px, i2 * py) == cur_img.get_pixel(j2 * px, i2 * py) {
                            continue;
                        }
                        let mut rank = 0;
                        if approx_target.get_pixel(j1 * px, i1 * py) == cur_img.get_pixel(j2 * px, i2 * py) {
                            rank += 1;
                        }
                        if approx_target.get_pixel(j2 * px, i2 * py) == cur_img.get_pixel(j1 * px, i1 * py) {
                            rank += 1;
                        }
                        if rank > best_rank {
                            best_rank = rank;
                            best_pair = ((i1, j1), (i2, j2));
                        }
                    }
                }
            }
        }
        if best_rank == 0 {
            break;
        }
        let ((i1, j1), (i2, j2)) = best_pair;
        let c1 = cur_img.get_pixel(j1 * px, i1 * py);
        let c2 = cur_img.get_pixel(j2 * px, i2 * py);
        cur_img.set_pixel(j1 * px, i1 * py, c2);
        cur_img.set_pixel(j2 * px, i2 * py, c1);

        let shape1 = Shape {
            x1: j1 * px,
            y1: i1 * py,
            x2: (j1 + 1) * px,
            y2: (i1 + 1) * py,
        };
        let shape2 = Shape {
            x1: j2 * px,
            y1: i2 * py,
            x2: (j2 + 1) * px,
            y2: (i2 + 1) * py,
        };
        let id1 = painter.blocks.iter().find(|(_id, b)| b.shape.contains(shape1)).unwrap().0.clone();
        let (id1, moves) = seg_util::isolate_rect(&mut painter, id1, shape1);
        all_moves.extend(moves);

        let id2 = painter.blocks.iter().find(|(_id, b)| b.shape.contains(shape2)).unwrap().0.clone();
        let (id2, moves) = seg_util::isolate_rect(&mut painter, id2, shape2);
        all_moves.extend(moves);

        let m = Swap { block_id1: id1, block_id2: id2 };
        painter.apply_move(&m);
        all_moves.push(m);

        let (_new_root, moves) = seg_util::merge_all(&mut painter);
        all_moves.extend(moves);
    }

    // eprintln!("---");
    // for m in &all_moves {
    //     eprintln!("{}", m);
    // }
    // eprintln!("---");

    let final_img = painter.render();
    final_img.save(&project_path("outputs/swan_hz.png"));
    // final_img.save(&project_path(format!("outputs/swan_{}_yo.png", problem_id)));
    let total_score = painter.cost + image_distance(&problem.target, &final_img).round() as i64;

    eprintln!("  {:?}: {}", args, total_score);
    (total_score, all_moves)
}