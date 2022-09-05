#![allow(unused_imports)]

use fxhash::FxHashMap as HashMap;
use fxhash::FxHashSet as HashSet;
use std::collections::BTreeMap;
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
use crate::transform::Transformation::TransposeXY;
use crate::transform::transform_solution;

use crate::basic::Move::*;

struct Shared {
    client: postgres::Client,
    best_scores: BTreeMap<i32, i64>,
    improvements: HashMap<i32, (SolverArgs, Vec<Move>)>,
}

#[derive(serde::Serialize)]
#[derive(Clone)]
struct SolverArgs {
    transposed: bool,
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
    let shared = Shared {
        client,
        best_scores,
        improvements: HashMap::default(),
    };
    let shared = Mutex::new(shared);

    std::thread::scope(|scope| {
        // imrovement submitter thread
        scope.spawn(|| {
            let start = std::time::Instant::now();
            loop {
                std::thread::sleep(std::time::Duration::from_secs(60));
                let shared = &mut *shared.lock().unwrap();
                eprintln!("submitting {} improvements", shared.improvements.len());
                let mut tx = shared.client.transaction().unwrap();
                let incovation_id = record_this_invocation(&mut tx, Status::KeepRunning { seconds: 65.0 });
                if !shared.improvements.is_empty() {
                    for (problem_id, (solver_args, moves)) in shared.improvements.drain() {
                        if dry_run {
                            eprintln!("dry run: pretend submit improvement for problem {}", problem_id);
                        } else {
                            upload_solution(&mut tx, problem_id, &moves, "brick", &serde_json::to_value(&solver_args).unwrap(), incovation_id);
                        }
                    }
                }
                tx.commit().unwrap();
                eprintln!("{}", crate::stats::STATS.render());

                for (problem_id, best_score) in &shared.best_scores {
                    eprintln!("problem {}:  our {},  best {}", problem_id, local_best_scores[problem_id].load(SeqCst), best_score);
                }

                let q = crate::stats_timer!("simulate_bricks").count.get() as f64 / start.elapsed().as_secs_f64();
                eprintln!("{} simulate_bricks per second", q.round() as i64);
                eprintln!();
            }
        });

        for problem_id in problem_range {
            for transposed in [true, false] {
                let local_best_score = &local_best_scores[&problem_id];
                let shared = &shared;
                scope.spawn(move || {
                    let mut wcache = WCache::new();
                    let mut problem = Problem::load(problem_id);
                    if transposed {
                        problem = problem.transform(&TransposeXY);
                    }

                    let mut painter = PainterState::new(&problem);
                    let (_, initial_moves) = seg_util::merge_all(&mut painter);
                    let initial_cost = painter.cost;
                    let mut best_blueprint = Blueprint::random();
                    loop {
                        let blueprint = if thread_rng().gen_bool(0.5) {
                            let mut b = best_blueprint.clone();
                            b.mutate();
                            b
                        } else {
                            Blueprint::random()
                        };

                        let mut sbr = simulate_bricks(&problem, blueprint.clone(), &mut wcache);
                        sbr.cost += initial_cost;
                        if thread_rng().gen_bool(0.01) {
                            let br = do_bricks(&problem, &initial_moves, blueprint.clone());
                            assert_eq!(sbr.cost, br.cost);
                            assert!((sbr.dist - br.dist).abs() <= 1);  // just in case there are rounding errors
                        }

                        // eprintln!("{} {}", problem_id, score);
                        if sbr.score() < local_best_score.load(SeqCst) {
                            local_best_score.store(sbr.score(), SeqCst);
                            eprintln!("improvement for problem {}: {}", problem_id, sbr.score());

                            let br = do_bricks(&problem, &initial_moves, blueprint.clone());
                            assert_eq!(sbr.cost, br.cost);
                            assert!((sbr.dist - br.dist).abs() <= 1);  // just in case there are rounding errors

                            best_blueprint = blueprint.clone();
                            let shared = &mut *shared.lock().unwrap();
                            let best_score = shared.best_scores.get_mut(&problem_id).unwrap();
                            if br.score < *best_score {
                                eprintln!("new best score for problem {}: {} -> {}", problem_id, *best_score, br.score);
                                let mut moves = br.moves;
                                if transposed {
                                    moves = transform_solution(&moves, &TransposeXY);
                                }
                                *best_score = br.score;
                                let a = SolverArgs {
                                    transposed,
                                };
                                shared.improvements.insert(problem_id, (a, moves));
                            }
                        }
                    }
                });
            }
        }
    });
}

fn random_seps() -> Vec<i32> {
    let mut res: HashSet<i32> = HashSet::default();
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

fn mutate_sep(seps: &mut Vec<i32>) {
    seps.remove(thread_rng().gen_range(0..seps.len()));
    loop {
        let x = thread_rng().gen_range(0..401);
        if !seps.contains(&x) {
            seps.push(x);
            break;
        }
    }
    seps.sort();
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
        if thread_rng().gen_bool(0.3) {
            let i = thread_rng().gen_range(0..self.xss.len());
            mutate_sep(&mut self.xss[i]);
            return;
        }
        if thread_rng().gen_bool(0.3) {
            mutate_sep(&mut self.ys);
            return;
        }
        let i = thread_rng().gen_range(0..self.xss.len());
        self.xss[i] = random_seps();
    }
}

fn to_recipe(mut xs: Vec<i32>) -> Vec<i32> {
    let mut res = vec![];
    loop {
        let left = xs[1] - xs[0];
        let right = xs[xs.len() - 1] - xs[xs.len() - 2];
        if left < right {
            res.push(left);
            xs.remove(0);
        } else {
            res.push(-right);
            xs.pop().unwrap();
        }
        if xs.len() == 1 {
            return res;
        }
    }
}

#[allow(dead_code)]
struct SimulateBrickResult {
    dist: i64,
    cost: i64,
}

impl SimulateBrickResult {
    fn score(&self) -> i64 { self.dist + self.cost }
}

fn simulate_bricks(problem: &Problem, blueprint: Blueprint, wcache: &mut WCache) -> SimulateBrickResult {
    let _t = crate::stats_timer!("simulate_bricks").time_it();
    let Blueprint { mut ys, mut xss } = blueprint;

    let mut cost = 0;
    let mut dist = 0.0;

    ys[0] = 0;
    *ys.last_mut().unwrap() = problem.target.height;

    loop {
        let h = *ys.last().unwrap() - ys[0];
        let y1;
        let y2;
        let mut xs;
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

        xs[0] = 0;
        *xs.last_mut().unwrap() = problem.target.width;

        let xs = to_recipe(xs);

        let mut merge_cost = 0;

        let mut xx1 = 0;
        let mut xx2 = problem.width;

        let cnt = xs.len();
        for (i, sx) in xs.into_iter().enumerate() {
            let x1;
            let x2;
            if sx > 0 {
                x1 = xx1;
                x2 = xx1 + sx;
            } else {
                x1 = xx2 + sx;
                x2 = xx2;
            }
            dist += wcache.get_dist(Shape { x1, y1, x2, y2 }, problem);
            cost += problem.cost(problem.base_costs.color, (xx2 - xx1) * h);

            if i + 1 == cnt {
                break;
            }

            cost += problem.cost(problem.base_costs.lcut, (xx2 - xx1) * h);
            merge_cost += problem.cost(problem.base_costs.merge, (x2 - x1).max((xx2 - xx1) - (x2 - x1)) * h);

            if sx > 0 {
                xx1 += sx;
            } else {
                xx2 += sx;
            }
        }

        if ys.len() == 1 {
            break;
        }

        cost += merge_cost;

        let dc = problem.cost(problem.base_costs.lcut, problem.width * h);
        cost += dc;
    }

    let dist = (dist * 0.005).round() as i64;

    SimulateBrickResult {
        cost,
        dist,
    }
}

#[allow(dead_code)]
struct BrickResult {
    score: i64,
    dist: i64,
    cost: i64,
    moves: Vec<Move>,
}
fn do_bricks(problem: &Problem, initial_moves: &[Move], blueprint: Blueprint) -> BrickResult {
    let _t = crate::stats_timer!("do_bricks").time_it();
    let mut painter = PainterState::new(problem);
    for m in initial_moves {
        painter.apply_move(m);
    }
    assert_eq!(painter.blocks.len(), 1);
    let mut block_id = painter.blocks.keys().next().unwrap().clone();
    let Blueprint { mut ys, mut xss } = blueprint;

    let mut cost = painter.cost;
    let mut dist = 0.0;

    // For simplicity, so we don't have to compute cost on margins.
    ys[0] = 0;
    *ys.last_mut().unwrap() = problem.target.height;

    /*
    let y1 = ys[0];
    let y2 = *ys.last().unwrap();
    if y1 < problem.target.height - y2 {
        if y1 > 0 {
            // cut bottom margin
            block_id = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Horizontal,
                line_number: y1,
            }).new_block_ids[1].clone();
        }
        if y2 < problem.target.height {
            // cut top margin
            block_id = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Horizontal,
                line_number: y2,
            }).new_block_ids[0].clone();
        }
    } else {
        if y2 < problem.target.height {
            // cut top margin
            block_id = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Horizontal,
                line_number: y2,
            }).new_block_ids[0].clone();
        }
        if y1 > 0 {
            // cut bottom margin
            block_id = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Horizontal,
                line_number: y1,
            }).new_block_ids[1].clone();
        }
    }*/

    loop {
        let h = *ys.last().unwrap() - ys[0];
        let y1;
        let y2;
        let mut xs;
        let cut_bottom;
        if ys[1] - ys[0] < ys[ys.len() - 1] - ys[ys.len() - 2] {
            cut_bottom = true;
            y1 = ys[0];
            y2 = ys[1];
            ys.remove(0);
            xs = xss.remove(0);
        } else {
            cut_bottom = false;
            y1 = ys[ys.len() - 2];
            y2 = ys[ys.len() - 1];
            ys.pop().unwrap();
            xs = xss.pop().unwrap();
        }

        let mut merge_stack: Vec<BlockId> = vec![];

        // Because we can't support different left and right margins
        // for different rows. They interfere.
        xs[0] = 0;
        *xs.last_mut().unwrap() = problem.target.width;

        /*
        // horizontal prunning
        let x1 = xs[0];
        let x2 = *xs.last().unwrap();

        let left_to_right = x1 < problem.target.width - x2;
        if left_to_right && x1 > 0 {
            // cut left margin
            let mut ids = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Vertical,
                line_number: x1,
            }).new_block_ids;
            block_id = ids.pop().unwrap();
            merge_stack.push(ids.pop().unwrap());
        }
        if x2 < problem.target.width {
            // cut right margin
            let mut ids = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Vertical,
                line_number: x2,
            }).new_block_ids;
            merge_stack.push(ids.pop().unwrap());
            block_id = ids.pop().unwrap();
        }
        if !left_to_right && x1 > 0 {
            // cut left margin
            let mut ids = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Vertical,
                line_number: x1,
            }).new_block_ids;
            block_id = ids.pop().unwrap();
            merge_stack.push(ids.pop().unwrap());
        }*/

        let mut merge_cost = 0;

        // horizontal cutting
        loop {
            let w = *xs.last().unwrap() - xs[0];
            let cut_left;
            let x1;
            let x2;
            if xs[1] - xs[0] < xs[xs.len() - 1] - xs[xs.len() - 2] {
                cut_left = true;
                x1 = xs[0];
                x2 = xs[1];
                xs.remove(0);
            } else {
                cut_left = false;
                x1 = xs[xs.len() - 2];
                x2 = xs[xs.len() - 1];
                xs.pop().unwrap();
            }

            let cf = crate::color_util::color_freqs(&problem.target, &Shape { x1, y1, x2, y2 });
            let color = crate::color_util::optimal_color_for_color_freqs(&cf);
            dist += crate::color_util::color_freqs_distance(&cf, color);

            let m = ColorMove { block_id: block_id.clone(), color };
            let dc2 = painter.apply_move(&m).cost;
            let dc = problem.cost(problem.base_costs.color, w * h);
            assert_eq!(painter.blocks[&block_id].shape.size(), w * h);
            assert_eq!(dc, dc2);
            cost += dc;

            if xs.len() == 1 {
                break;
            }

            let dc = problem.cost(problem.base_costs.lcut, w * h);
            cost += dc;
            merge_cost += problem.cost(problem.base_costs.merge, (x2 - x1).max(w - (x2 - x1)) * h);

            if cut_left {
                let mut res = painter.apply_move(&Move::LCut {
                    block_id,
                    orientation: Orientation::Vertical,
                    line_number: x2,
                });
                assert_eq!(dc, res.cost);
                block_id = res.new_block_ids.pop().unwrap();
                merge_stack.push(res.new_block_ids.pop().unwrap());
            } else {
                let mut res = painter.apply_move(&Move::LCut {
                    block_id,
                    orientation: Orientation::Vertical,
                    line_number: x1,
                });
                assert_eq!(dc, res.cost);
                merge_stack.push(res.new_block_ids.pop().unwrap());
                block_id = res.new_block_ids.pop().unwrap();
            }
        }

        if ys.len() == 1 {
            break;
        }

        // horizonal merging
        while let Some(id) = merge_stack.pop() {
            block_id = painter.apply_move(&Move::Merge {
                block_id1: block_id,
                block_id2: id,
            }).new_block_ids[0].clone();
        }
        cost += merge_cost;

        let dc = problem.cost(problem.base_costs.lcut, problem.width * h);
        cost += dc;

        if cut_bottom {
            let mut res = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Horizontal,
                line_number: y2,
            });
            block_id = res.new_block_ids.pop().unwrap();
            assert_eq!(dc, res.cost);
        } else {
            let mut res = painter.apply_move(&Move::LCut {
                block_id,
                orientation: Orientation::Horizontal,
                line_number: y1,
            });
            res.new_block_ids.pop().unwrap();
            block_id = res.new_block_ids.pop().unwrap();
            assert_eq!(dc, res.cost);
        }
    }

    let dist1 = (dist * 0.005).round() as i64;

    let img = painter.render();
    let dist = image_distance(&problem.target, &img).round() as i64;
    let score = painter.cost + dist;

    assert_eq!(dist1, dist);

    assert_eq!(cost, painter.cost);

    BrickResult {
        score,
        cost: painter.cost,
        dist,
        moves: painter.moves,
    }

    /*let br2 = do_bricks_naive(problem, initial_moves, blueprint);

    assert_eq!(br.dist, br2.dist);
    if br.cost != br2.cost {
        eprintln!("cost discrepancy: {} vs {} ({})", br.cost, br2.cost, br.cost - br2.cost);
    }*/

    // br
}

#[allow(dead_code)]
fn do_bricks_naive(problem: &Problem, initial_moves: &[Move], blueprint: Blueprint) -> BrickResult {
    let _t = crate::stats_timer!("do_bricks_naive").time_it();
    let mut painter = PainterState::new(problem);
    for m in initial_moves {
        painter.apply_move(m);
    }
    assert_eq!(painter.blocks.len(), 1);
    let mut block_id = painter.blocks.keys().next().unwrap().clone();
    let Blueprint { mut ys, mut xss } = blueprint;

    loop {
        let (new_block, _moves) = seg_util::isolate_rect(&mut painter, block_id, Shape {
            x1: 0,
            y1: ys[0],
            x2: problem.target.width,
            y2: *ys.last().unwrap(),
        });
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
            let (new_block, _moves) = seg_util::isolate_rect(&mut painter, block_id, Shape {
                x1: xs[0],
                y1: y_min,
                x2: *xs.last().unwrap(),
                y2: y_max,
            });
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
            if xs.len() == 1 {
                break;
            }
        }

        if ys.len() == 1 {
            break;
        }

        let (new_block, _moves) = seg_util::merge_all_inside_bb(&mut painter, Shape {
            x1: 0,
            y1: y_min,
            x2: problem.target.width,
            y2: y_max,
        });
        block_id = new_block;
    }

    let img = painter.render();
    let dist = image_distance(&problem.target, &img).round() as i64;
    let score = painter.cost + dist;

    BrickResult {
        score,
        cost: painter.cost,
        dist,
        moves: painter.moves,
    }
}

struct WCache {
    shape_to_dist: HashMap<Shape, f64>,
}

impl WCache {
    fn new() -> WCache {
        WCache {
            shape_to_dist: HashMap::default(),
        }
    }

    fn get_dist(&mut self, shape: Shape, problem: &Problem) -> f64 {
        if let Some(&dist) = self.shape_to_dist.get(&shape) {
            crate::stats_counter!("wcache/hit").inc();
            return dist;
        }
        crate::stats_counter!("wcache/miss").inc();
        let cf = crate::color_util::color_freqs(&problem.target, &shape);
        let color = crate::color_util::optimal_color_for_color_freqs(&cf);
        let dist = crate::color_util::color_freqs_distance(&cf, color);
        self.shape_to_dist.insert(shape, dist);
        dist
    }
}
