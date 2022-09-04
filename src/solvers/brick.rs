#![allow(unused_imports)]

use std::collections::{HashSet, HashMap, BTreeMap};
use std::sync::Mutex;
use std::sync::atomic::{AtomicI64, Ordering::SeqCst};
use rand::prelude::*;
use crate::util::project_path;
use crate::basic::*;
use crate::image::Image;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;
use crate::color_util::*;
use crate::seg_util;

use crate::basic::Move::*;

struct Shared {
    client: postgres::Client,
    best_scores: BTreeMap<i32, i64>,
    improvements: HashMap<i32, Vec<Move>>,
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
    let local_best_scores: BTreeMap<i32, AtomicI64> = problem_range.clone().map(|i| (i, AtomicI64::new(i64::MAX))).collect();
    let improvements: HashMap<i32, Vec<Move>> = HashMap::new();
    let shared = Shared {
        client, best_scores, improvements,
    };
    let shared = Mutex::new(shared);

    std::thread::scope(|scope| {
        // imrovement submitter thread
        scope.spawn(|| {
            loop {
                std::thread::sleep(std::time::Duration::from_secs(60));
                let shared = &mut *shared.lock().unwrap();
                eprintln!("submitting {} improvements", shared.improvements.len());
                let mut tx = shared.client.transaction().unwrap();
                let incovation_id = record_this_invocation(&mut tx, Status::KeepRunning { seconds: 65.0 });
                if !shared.improvements.is_empty() {
                    for (problem_id, moves) in shared.improvements.drain() {
                        if dry_run {
                            eprintln!("dry run: pretend submit improvement for problem {}", problem_id);
                        } else {
                            upload_solution(&mut tx, problem_id, &moves, "brick", &serde_json::Value::Null, incovation_id);
                        }
                    }
                }
                tx.commit().unwrap();
                eprintln!("{}", crate::stats::STATS.render());

                for (problem_id, best_score) in &shared.best_scores {
                    eprintln!("problem {}:  our {},  best {}", problem_id, local_best_scores[problem_id].load(SeqCst), best_score);
                }
            }
        });

        for problem_id in problem_range {
            let local_best_score = &local_best_scores[&problem_id];
            let shared = &shared;
            scope.spawn(move || {
                let problem = Problem::load(problem_id);
                let mut painter = PainterState::new(&problem);
                let (_, initial_moves) = seg_util::merge_all(&mut painter);
                let mut best_blueprint = Blueprint::random();
                loop {
                    let blueprint = if thread_rng().gen_bool(0.5) {
                        let mut b = best_blueprint.clone();
                        b.mutate();
                        b
                    } else {
                        Blueprint::random()
                    };
                    let (score, moves) = do_bricks(&problem, &initial_moves, blueprint.clone());
                    // eprintln!("{} {}", problem_id, score);
                    if score < local_best_score.load(SeqCst) {
                        local_best_score.store(score, SeqCst);
                        eprintln!("improvement for problem {}: {}", problem_id, score);
                        best_blueprint = blueprint;
                        let shared = &mut *shared.lock().unwrap();
                        let best_score = shared.best_scores.get_mut(&problem_id).unwrap();
                        if score < *best_score {
                            eprintln!("new best score for problem {}: {} -> {}", problem_id, *best_score, score);
                            *best_score = score;
                            shared.improvements.insert(problem_id, moves);
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

#[derive(Clone)]
struct Blueprint {
    ys: Vec<i32>,
    xss: Vec<Vec<i32>>,
}

impl Blueprint {
    fn random() -> Blueprint {
        let ys = random_seps();
        let mut xss = vec![];
        for _ in 1..ys.len() {
            xss.push(random_seps());
        }
        Blueprint { ys, xss }
    }

    fn mutate(&mut self) {
        let i = thread_rng().gen_range(0..self.xss.len());
        self.xss[i] = random_seps();
    }
}

fn do_bricks(problem: &Problem, initial_moves: &[Move], blueprint: Blueprint) -> (i64, Vec<Move>) {
    let _t = crate::stats_timer!("do_bricks").time_it();
    let mut painter = PainterState::new(problem);
    let mut all_moves = vec![];

    for m in initial_moves {
        painter.apply_move(m);
        all_moves.push(m.clone());
    }

    assert_eq!(painter.blocks.len(), 1);
    let mut block_id = painter.blocks.keys().next().unwrap().clone();

    let Blueprint { mut ys, mut xss } = blueprint;
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
