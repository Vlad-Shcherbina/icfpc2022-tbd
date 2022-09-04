#![allow(unused_imports)]

use std::collections::{HashSet, HashMap, BTreeMap};
use std::sync::Mutex;
use rand::prelude::*;
use crate::util::project_path;
use crate::basic::*;
use crate::image::Image;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;
use crate::color_util::*;
use crate::seg_util;

use crate::basic::Move::*;


crate::entry_point!("brick_solver", brick_solver);
fn brick_solver() {
    let mut pargs = pico_args::Arguments::from_vec(std::env::args_os().skip(2).collect());
    let problems: String = pargs.value_from_str("--problem").unwrap();
    let dry_run = pargs.contains("--dry-run");
    let rest = pargs.finish();
    assert!(rest.is_empty(), "unrecognized arguments {:?}", rest);
    let problem_range = crate::util::parse_range(&problems);

    let mut client = crate::db::create_client();
    let rows = client.query("SELECT problem_id, moves_cost + image_distance AS score FROM solutions", &[]).unwrap();
    let mut best_scores: BTreeMap<i32, i64> = problem_range.clone().map(|i| (i, i64::MAX)).collect();
    for row in rows {
        let problem_id: i32 = row.get("problem_id");
        let score: i64 = row.get("score");
        if let Some(e) = best_scores.get_mut(&problem_id) {
            *e = (*e).min(score);
        }
    }
    eprintln!("{:?}", best_scores);
    let local_best_scores: BTreeMap<i32, Mutex<i64>> = problem_range.clone().map(|i| (i, Mutex::new(i64::MAX))).collect();
    let improvements: HashMap<i32, Vec<Move>> = HashMap::new();

    let client = Mutex::new(client);
    let improvements = Mutex::new(improvements);
    let best_scores: BTreeMap<_, _> = best_scores.into_iter().map(|(k, v)| (k, Mutex::new(v))).collect();
    std::thread::scope(|scope| {
        // imrovement submitter thread
        scope.spawn(|| {
            loop {
                std::thread::sleep(std::time::Duration::from_secs(60));
                let improvements = &mut *improvements.lock().unwrap();
                eprintln!("submitting {} improvements", improvements.len());
                let client = &mut *client.lock().unwrap();
                let mut tx = client.transaction().unwrap();
                let incovation_id = record_this_invocation(&mut tx, Status::KeepRunning { seconds: 65.0 });
                if !improvements.is_empty() {
                    for (problem_id, moves) in improvements.drain() {
                        if dry_run {
                            eprintln!("dry run: pretend submit improvement for problem {}", problem_id);
                        } else {
                            upload_solution(&mut tx, problem_id, &moves, "brick", &serde_json::Value::Null, incovation_id);
                        }
                    }
                }
                tx.commit().unwrap();
                eprintln!("{}", crate::stats::STATS.render());

                for (problem_id, best_score) in &best_scores {
                    eprintln!("problem {}:  our {},  best {}", problem_id, local_best_scores[problem_id].lock().unwrap(), best_score.lock().unwrap());
                }
            }
        });

        for problem_id in problem_range {
            let local_best_score = &local_best_scores[&problem_id];
            let best_score = &best_scores[&problem_id];
            let improvements = &improvements;
            scope.spawn(move || {
                let problem = Problem::load(problem_id);
                let mut painter = PainterState::new(&problem);
                let (_, initial_moves) = seg_util::merge_all(&mut painter);
                loop {
                    let ys = random_seps();
                    let mut xss = vec![];
                    for _ in 1..ys.len() {
                        xss.push(random_seps());
                    }
                    let (score, moves) = do_bricks(&problem, &initial_moves, ys, xss);
                    // eprintln!("{} {}", problem_id, score);
                    let local_best_score = &mut *local_best_score.lock().unwrap();
                    if score < *local_best_score {
                        eprintln!("improvement for problem {}: {} -> {}", problem_id, *local_best_score, score);
                        *local_best_score = score;
                        let best_score = &mut *best_score.lock().unwrap();
                        if score < *best_score {
                            eprintln!("new best score for problem {}: {} -> {}", problem_id, *best_score, score);
                            *best_score = score;
                            let improvements = &mut *improvements.lock().unwrap();
                            improvements.insert(problem_id, moves);
                        }
                    }
                }
            });
        }
    });
}

fn random_seps() -> Vec<i32> {
    let mut res: HashSet<i32> = HashSet::new();
    if thread_rng().gen_bool(0.9) {
        res.insert(0);
        res.insert(400);
    }
    loop {
        res.insert(thread_rng().gen_range(0..401));
        if res.len() >= 2 && thread_rng().gen_bool(0.3) {
            break;
        }
    }
    let mut res: Vec<i32> = res.into_iter().collect();
    res.sort();
    res
}

fn do_bricks(problem: &Problem, initial_moves: &[Move], mut ys: Vec<i32>, mut xss: Vec<Vec<i32>>) -> (i64, Vec<Move>) {
    let _t = crate::stats_timer!("do_bricks").time_it();
    let mut painter = PainterState::new(problem);
    let mut all_moves = vec![];

    for m in initial_moves {
        painter.apply_move(m);
        all_moves.push(m.clone());
    }

    assert_eq!(painter.blocks.len(), 1);
    let mut block_id = painter.blocks.keys().next().unwrap().clone();

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

    let img = painter.render();
    let score = painter.cost + image_distance(&problem.target, &img).round() as i64;

    (score, all_moves)
}
