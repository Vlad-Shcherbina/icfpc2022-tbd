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

#[derive(serde::Serialize, serde::Deserialize)]
#[derive(Debug, Clone)]
struct SolverArgs {
    transposed: bool,
    granularity: i32,
    ys: Vec<i32>,
}

crate::entry_point!("brick_solver", brick_solver, _EP1);
fn brick_solver() {
    let mut pargs = pico_args::Arguments::from_vec(std::env::args_os().skip(2).collect());
    let problems: String = pargs.value_from_str("--problem").unwrap();
    let dry_run = pargs.contains("--dry-run");
    let granularity: i32 = pargs.value_from_str("--granularity").unwrap();
    let start_from_best = pargs.contains("--start-from-best");
    let rest = pargs.finish();
    assert!(rest.is_empty(), "unrecognized arguments {:?}", rest);
    let problem_range = crate::util::parse_range(&problems);

    assert_eq!(400 % granularity, 0);

    let mut client = crate::db::create_client();
    let rows = client.query("SELECT problem_id, moves_cost + image_distance AS score, solver_args FROM solutions", &[]).unwrap();
    let mut best_scores: BTreeMap<i32, i64> = problem_range.clone().map(|i| (i, i64::MAX)).collect();
    let mut best_solver_args: HashMap<i32, SolverArgs> = HashMap::default();
    for row in rows {
        let problem_id: i32 = row.get("problem_id");
        let score: i64 = row.get("score");
        let postgres::types::Json::<serde_json::Value>(sa) = row.get("solver_args");

        if let Some(e) = best_scores.get_mut(&problem_id) {
            *e = (*e).min(score);

            if let Ok(sa) = serde_json::from_value::<SolverArgs>(sa) {
                best_solver_args.insert(problem_id, sa);
            }
        }
    }
    eprintln!("{:?}", best_scores);
    eprintln!("{:?}", best_solver_args);

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
                            upload_solution(&mut tx, problem_id, &moves, "brick DP", &serde_json::to_value(&solver_args).unwrap(), incovation_id);
                        }
                    }
                }
                tx.commit().unwrap();
                eprintln!("{}", crate::stats::STATS.render());

                for (problem_id, best_score) in &shared.best_scores {
                    eprintln!("problem {}:  our {},  best {}", problem_id, local_best_scores[problem_id].load(SeqCst), best_score);
                }

                let q = crate::stats_timer!("simulate_bricks").count.get() as f64 / start.elapsed().as_secs_f64();
                eprintln!("{} simulate_bricks per second", q);
                eprintln!();
            }
        });

        for problem_id in problem_range {
            for transposed in [true, false] {
                let local_best_score = &local_best_scores[&problem_id];
                let shared = &shared;
                let best_solver_args = &best_solver_args;
                scope.spawn(move || {
                    let mut dp_cache: DpCache = HashMap::default();
                    let mut wcache = WCache::new();
                    let mut problem = Problem::load(problem_id);
                    if transposed {
                        problem = problem.transform(&TransposeXY);
                    }

                    let mut painter = PainterState::new(&problem);
                    let (_, initial_moves) = seg_util::merge_all(&mut painter);
                    let initial_cost = painter.cost;
                    let mut best_ys = random_seps();
                    if start_from_best {
                        if let Some(sa) = best_solver_args.get(&problem_id) {
                            if sa.transposed == transposed {
                                best_ys = sa.ys.clone();
                                eprintln!("problem {}: start from best ys {:?}", problem_id, best_ys);
                            } else {
                                return;
                            }
                        }
                    }

                    loop {
                        let ys = if thread_rng().gen_bool(0.1) {
                            random_seps()
                        } else {
                            let mut ys = best_ys.clone();
                            mutate_sep(&mut ys);
                            ys
                        };

                        let (dp_score, xss) = dp(&problem, ys.clone(), granularity, &mut wcache, &mut dp_cache);

                        let mut sbr = simulate_bricks(&problem, ys.clone(), xss.clone(), &mut wcache);
                        assert!((sbr.score() as f64 - dp_score) <= 2.0);
                        // eprintln!("*********** {} {}", sbr.score(), dp_score);

                        sbr.cost += initial_cost;
                        if thread_rng().gen_bool(0.01) {
                            let br = do_bricks(&problem, &initial_moves, ys.clone(), xss.clone());
                            assert_eq!(sbr.cost, br.cost);
                            assert!((sbr.dist - br.dist).abs() <= 1);  // just in case there are rounding errors
                        }

                        // eprintln!("{} {}", problem_id, score);
                        if sbr.score() < local_best_score.load(SeqCst) {
                            local_best_score.store(sbr.score(), SeqCst);
                            eprintln!("improvement for problem {}: {}   {:?}", problem_id, sbr.score(), ys);

                            let br = do_bricks(&problem, &initial_moves, ys.clone(), xss.clone());
                            assert_eq!(sbr.cost, br.cost);
                            assert!((sbr.dist - br.dist).abs() <= 1);  // just in case there are rounding errors

                            best_ys = ys.clone();
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
                                    granularity,
                                    ys: ys.clone(),
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
    if thread_rng().gen_bool(0.5) {
        let old_x = seps.remove(thread_rng().gen_range(0..seps.len()));
        loop {
            let range = if thread_rng().gen_bool(0.5) {
                0..401
            } else {
                (old_x - 10).max(0)..(old_x + 10).min(401)
            };
            let x = thread_rng().gen_range(range);
            if !seps.contains(&x) {
                seps.push(x);
                break;
            }
        }
    } else if seps.len() > 2 && thread_rng().gen_bool(0.5) {
        seps.remove(thread_rng().gen_range(0..seps.len()));
    } else {
        loop {
            let x = thread_rng().gen_range(0..401);
            if !seps.contains(&x) {
                seps.push(x);
                break;
            }
        }
    }
    seps.sort();
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

crate::entry_point!("dp_demo", dp_demo, _EP2);
fn dp_demo() {
    let granularity = 10;
    let start = std::time::Instant::now();
    let problem = Problem::load(17);
    let mut wcache = WCache::new();
    let ys = vec![0, 200, 400];

    let (score, xss) = dp(&problem, ys.clone(), granularity, &mut wcache, &mut HashMap::default());
    eprintln!("score: {}", score);
    for xs in &xss {
        eprintln!("xs = {:?}", xs)
    }

    let sbr = simulate_bricks(&problem, ys.clone(), xss.clone(), &mut wcache);
    dbg!(sbr.score());

    let br = do_bricks(&problem, &[], ys, xss);
    dbg!(br.score);


    let mut painter = PainterState::new(&problem);
    for m in &br.moves {
        painter.apply_move(m);
    }
    painter.render().save(&project_path("outputs/dp.png"));


    eprintln!("{}", crate::stats::STATS.render());
    eprintln!("it took {:?}", start.elapsed());
}

fn dp(problem: &Problem, mut ys: Vec<i32>, granularity: i32, wcache: &mut WCache, dp_cache: &mut DpCache) -> (f64, Vec<Vec<i32>>) {
    let _t = crate::stats_timer!("dp").time_it();
    let mut score = 0.0;
    ys[0] = 0;
    *ys.last_mut().unwrap() = problem.target.height;

    let mut xss = vec![];

    let ys = to_recipe(ys);

    let mut yy1 = 0;
    let mut yy2 = problem.height;
    for (i, &sy) in ys.iter().enumerate() {
        let y1;
        let y2;
        if sy > 0 {
            y1 = yy1;
            y2 = yy1 + sy;
        } else {
            y2 = yy2;
            y1 = yy2 + sy;
        }

        let last = i + 1 == ys.len();
        let (row_score, xs) = row_dp(problem, y1, y2, yy2 - yy1, !last, granularity, wcache, dp_cache);
        xss.push(xs);
        score += row_score;

        if last {
            break;
        }
        score += problem.cost(problem.base_costs.lcut, problem.width * (yy2 - yy1)) as f64;

        if sy > 0 {
            yy1 += sy;
        } else {
            yy2 += sy;
        }
    }

    (score, xss)
}

type DpCache = HashMap<(i32, i32, i32, bool), (f64, Vec<i32>)>;

#[allow(clippy::too_many_arguments)]
fn row_dp(problem: &Problem, y1: i32, y2: i32, h: i32, merge: bool, granularity: i32, wcache: &mut WCache, dp_cache: &mut DpCache) -> (f64, Vec<i32>) {
    let _t = crate::stats_timer!("row_dp").time_it();

    let cache_key = (y1, y2, h, merge);
    if let Some(e) = dp_cache.get(&cache_key) {
        crate::stats_counter!("row_dp/hit").inc();
        return e.clone();
    }
    crate::stats_counter!("row_dp/miss").inc();

    let mut best: HashMap<(i32, i32), (f64, Vec<i32>)> = HashMap::default();

    for w in 1..=problem.width {
        if w % granularity != 0 {
            continue;
        }
        // dbg!(w);
        for x1 in 0..=problem.width - w {
            if x1 % granularity != 0 {
                continue;
            }
            let x2 = x1 + w;
            let mut best_score =
                wcache.get_dist(Shape {x1, y1, x2, y2}, problem) * 0.005 +
                problem.cost(problem.base_costs.color, (x2 - x1) * h) as f64;
            let mut best_sol = vec![x2 - x1];

            let color_and_cut_cost =
                problem.cost(problem.base_costs.color, (x2 - x1) * h) as f64 +
                problem.cost(problem.base_costs.lcut, (x2 - x1) * h) as f64;
            for x in x1 + 1 .. x2 {
                if x % granularity != 0 {
                    continue;
                }
                // if !thread_rng().gen_bool(0.01) {
                //     continue;
                // }
                let merge_cost = if merge {
                    problem.cost(problem.base_costs.merge, h * (x - x1).max(x2 - x)) as f64
                } else {
                    0.0
                };

                let (score1, sol1) = &best[&(x, x2)];
                let score =
                    wcache.get_dist(Shape {x1, y1, x2: x, y2}, problem) * 0.005 +
                    color_and_cut_cost + merge_cost +
                    score1;
                if score < best_score {
                    best_score = score;
                    best_sol = sol1.clone();
                    best_sol.push(x - x1);
                }

                let (score2, sol2) = &best[&(x1, x)];
                let score =
                    wcache.get_dist(Shape {x1: x, y1, x2, y2}, problem) * 0.005 +
                    color_and_cut_cost + merge_cost +
                    score2;
                if score < best_score {
                    best_score = score;
                    best_sol = sol2.clone();
                    best_sol.push(x - x2);
                }
            }
            let old = best.insert((x1, x2), (best_score, best_sol));
            assert!(old.is_none());
        }
    }

    let (score, mut xs) = best.remove(&(0, problem.width)).unwrap();
    xs.reverse();
    let res = (score, xs);
    dp_cache.insert(cache_key, res.clone());
    res
}

impl SimulateBrickResult {
    fn score(&self) -> i64 { self.dist + self.cost }
}

fn simulate_bricks(problem: &Problem, mut ys: Vec<i32>, mut xss: Vec<Vec<i32>>, wcache: &mut WCache) -> SimulateBrickResult {
    let _t = crate::stats_timer!("simulate_bricks").time_it();

    let mut cost = 0;
    let mut dist = 0.0;

    ys[0] = 0;
    *ys.last_mut().unwrap() = problem.target.height;

    loop {
        let h = *ys.last().unwrap() - ys[0];
        let y1;
        let y2;
        if ys[1] - ys[0] < ys[ys.len() - 1] - ys[ys.len() - 2] {
            y1 = ys[0];
            y2 = ys[1];
            ys.remove(0);
            // xs = xss.remove(0);
        } else {
            y1 = ys[ys.len() - 2];
            y2 = ys[ys.len() - 1];
            ys.pop().unwrap();
            // xs = xss.pop().unwrap();
        }

        // xs[0] = 0;
        // *xs.last_mut().unwrap() = problem.target.width;

        // let xs = to_recipe(xs);
        let xs = xss.remove(0);

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
fn do_bricks(problem: &Problem, initial_moves: &[Move], mut ys: Vec<i32>, mut xss: Vec<Vec<i32>>) -> BrickResult {
    let _t = crate::stats_timer!("do_bricks").time_it();
    let mut painter = PainterState::new(problem);
    for m in initial_moves {
        painter.apply_move(m);
    }
    assert_eq!(painter.blocks.len(), 1);
    let mut block_id = painter.blocks.keys().next().unwrap().clone();

    let mut cost = painter.cost;
    let mut dist = 0.0;

    // For simplicity, so we don't have to compute cost on margins.
    ys[0] = 0;
    *ys.last_mut().unwrap() = problem.target.height;

    loop {
        let h = *ys.last().unwrap() - ys[0];
        let y1;
        let y2;
        // let mut xs;
        let cut_bottom;
        if ys[1] - ys[0] < ys[ys.len() - 1] - ys[ys.len() - 2] {
            cut_bottom = true;
            y1 = ys[0];
            y2 = ys[1];
            ys.remove(0);
            // xs = xss.remove(0);
        } else {
            cut_bottom = false;
            y1 = ys[ys.len() - 2];
            y2 = ys[ys.len() - 1];
            ys.pop().unwrap();
            // xs = xss.pop().unwrap();
        }

        let mut merge_stack: Vec<BlockId> = vec![];

        // Because we can't support different left and right margins
        // for different rows. They interfere.
        // xs[0] = 0;
        // *xs.last_mut().unwrap() = problem.target.width;
        // let xs = to_recipe(xs);
        let xs = xss.remove(0);

        let mut merge_cost = 0;

        let mut xx1 = 0;
        let mut xx2 = problem.width;
        // horizontal cutting

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

            let cf = crate::color_util::color_freqs(&problem.target, &Shape { x1, y1, x2, y2 });
            let color = crate::color_util::optimal_color_for_color_freqs(&cf);
            dist += crate::color_util::color_freqs_distance(&cf, color);

            let m = ColorMove { block_id: block_id.clone(), color };
            let dc2 = painter.apply_move(&m).cost;
            let dc = problem.cost(problem.base_costs.color, (xx2 - xx1) * h);
            assert_eq!(painter.blocks[&block_id].shape.size(), (xx2 - xx1) * h);
            assert_eq!(dc, dc2);
            cost += dc;

            if i + 1 == cnt {
                break;
            }

            let dc = problem.cost(problem.base_costs.lcut, (xx2 - xx1) * h);
            cost += dc;
            merge_cost += problem.cost(problem.base_costs.merge, (x2 - x1).max(xx2 - xx1 - (x2 - x1)) * h);

            if sx > 0 {
                let mut res = painter.apply_move(&Move::LCut {
                    block_id,
                    orientation: Orientation::Vertical,
                    line_number: x2,
                });
                assert_eq!(dc, res.cost);
                block_id = res.new_block_ids.pop().unwrap();
                merge_stack.push(res.new_block_ids.pop().unwrap());
                xx1 += sx;
            } else {
                let mut res = painter.apply_move(&Move::LCut {
                    block_id,
                    orientation: Orientation::Vertical,
                    line_number: x1,
                });
                assert_eq!(dc, res.cost);
                merge_stack.push(res.new_block_ids.pop().unwrap());
                block_id = res.new_block_ids.pop().unwrap();
                xx2 += sx;
            }
        }

        if ys.len() == 1 {
            break;
        }

        // horizontal merging
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
