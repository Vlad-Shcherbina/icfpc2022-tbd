#![allow(unused_imports)]

use fxhash::FxHashMap as HashMap;
use rand::prelude::*;
use crate::basic::*;
use crate::basic::Move::*;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;
use crate::color_util::optimal_color_for_color_freqs;
use crate::seg_util;
use crate::util::project_path;

crate::entry_point!("spot_solver", spot_solver);
fn spot_solver() {
    let problem_id = 18;
    let mut best_score = 1_000_000_000;
    let mut best_moves = vec![];
    let mut best_rects = vec![];

    for _iter in 0..1000 {
        // dbg!(iter);

        let problem = Problem::load(problem_id);

        let mut rects = vec![];
        if thread_rng().gen_bool(0.5) {
            rects.push(Shape { x1: 0, y1: 0, x2: 400, y2: 400 });
            for _ in 0..thread_rng().gen_range(0..10) {
                let w = thread_rng().gen_range(100..problem.target.width + 1);
                let h = thread_rng().gen_range(100..problem.target.height + 1);
                let x1 = thread_rng().gen_range(0..problem.target.width - w + 1);
                let y1 = thread_rng().gen_range(0..problem.target.height - h + 1);
                rects.push(Shape { x1, y1, x2: x1 + w, y2: y1 + h });
            }
        } else {
            rects = best_rects.clone();
            for rect in &mut rects {
                if thread_rng().gen_bool(0.5) {
                    continue;
                }
                rect.x1 += thread_rng().gen_range(-5..6) * thread_rng().gen_range(0..2);
                rect.y1 += thread_rng().gen_range(-5..6) * thread_rng().gen_range(0..2);
                rect.x2 += thread_rng().gen_range(-5..6) * thread_rng().gen_range(0..2);
                rect.y2 += thread_rng().gen_range(-5..6) * thread_rng().gen_range(0..2);

                rect.x1 = rect.x1.max(0).min(problem.target.width - 1);
                rect.y1 = rect.y1.max(0).min(problem.target.height - 1);
                rect.x2 = rect.x2.max(rect.x1 + 1).min(problem.target.width);
                rect.y2 = rect.y2.max(rect.y1 + 1).min(problem.target.height);
            }
        }

        let mut owners: Vec<Option<usize>> = vec![None; (problem.target.width * problem.target.height) as usize];

        for (i, rect) in rects.iter().enumerate() {
            for y in rect.y1..rect.y2 {
                for x in rect.x1..rect.x2 {
                    owners[(y * problem.target.width + x) as usize] = Some(i);
                }
            }
        }
        let mut color_freqss: Vec<HashMap<Color, f64>> = vec![HashMap::default(); rects.len()];
        for y in 0..problem.target.height {
            for x in 0..problem.target.width {
                let owner = owners[(y * problem.target.width + x) as usize];
                if let Some(owner) = owner {
                    *color_freqss[owner].entry(problem.target.get_pixel(x, y)).or_default() += 1.0;
                }
            }
        }
        let colors: Vec<Color> = color_freqss.iter().map(optimal_color_for_color_freqs).collect();

        let mut painter = PainterState::new(&problem);
        let mut all_moves = vec![];
        let mut root = BlockId::root(0);
        for i in 0..rects.len() {
            let (id, moves) = seg_util::isolate_rect(&mut painter, root, rects[i]);
            all_moves.extend(moves);

            let m = ColorMove { block_id: id, color: colors[i] };
            painter.apply_move(&m);
            all_moves.push(m);

            if i + 1 == rects.len() {
                break;
            }
            let (new_root, moves) = seg_util::merge_all(&mut painter);
            all_moves.extend(moves);
            root = new_root;
        }

        let score = painter.cost + image_distance(&painter.render(), &problem.target).round() as i64;

        if score < best_score {
            dbg!(score);
            best_score = score;
            best_moves = all_moves;
            best_rects = rects;
        }
    }

    let problem = Problem::load(problem_id);
    let mut painter = PainterState::new(&problem);
    for m in &best_moves {
        painter.apply_move(m);
    }
    painter.render().save(&project_path("outputs/spot.png"));
}
