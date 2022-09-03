use crate::util::project_path;
use crate::basic::*;
use crate::image::Image;
// use crate::invocation::{record_this_invocation, Status};
// use crate::uploader::upload_solution;
use crate::solver_utils::*;

use crate::basic::Move::*;

crate::entry_point!("swan_solver", swan_solver);
fn swan_solver() {
    let mut pargs = pico_args::Arguments::from_vec(std::env::args_os().skip(2).collect());
    let problems: String = pargs.value_from_str("--problem").unwrap();
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
    for problem_id in problem_range {
        eprintln!("*********** problem {} ***********", problem_id);
        let problem = Problem::load(problem_id);

        let px = 40;
        let py = 40;

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

        let mut palette = k_means(&color_freqss, 3);
        palette.sort();

        let mut approx_target = Image::new(problem.target.width, problem.target.height, Color::default());

        let mut color_cnts = vec![0; palette.len()];

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
            }
        }
        approx_target.save(&project_path(format!("outputs/swan_{}_approx.png", problem_id)));
        dbg!(&color_cnts);

        let mut sorted_target = Image::new(problem.target.width, problem.target.height, Color::default());
        let mut qq = 0;
        for (color_idx, &cnt) in color_cnts.iter().enumerate() {
            for _ in 0..cnt {
                let i = qq / w;
                let j = qq % w;
                let shape = Shape {
                    x1: j * px,
                    y1: i * py,
                    x2: (j + 1) * px,
                    y2: (i + 1) * py,
                };
                sorted_target.fill_rect(shape, palette[color_idx]);
                qq += 1;
            }
        }
        sorted_target.save(&project_path(format!("outputs/swan_{}_sorted.png", problem_id)));
        let mut rects: Vec<(Shape, Color)> = vec![];
        for i in 0..h {
            let mut color_idx = 0;
            let mut len = 0;
            for j in 0..w {
                let c = sorted_target.get_pixel(j * px, i * py);
                let ci = palette.iter().position(|&x| x == c).unwrap();
                if ci == color_idx {
                    len += 1;
                } else {
                    if len > 0 {
                        eprintln!("i={}, j1={}, j2={}, color={}", i, j - len, j, palette[color_idx]);
                        rects.push((Shape {
                            x1: (j - len) * px,
                            y1: i * py,
                            x2: j * px,
                            y2: (i + 1) * py,
                        }, palette[color_idx]));
                    }
                    color_idx = ci;
                    len = 1;
                }
            }
            if len > 0 {
                let j = w;
                eprintln!("i={}, j1={}, j2={}, color={}", i, j - len, j, palette[color_idx]);
                rects.push((Shape {
                    x1: (j - len) * px,
                    y1: i * py,
                    x2: j * px,
                    y2: (i + 1) * py,
                }, palette[color_idx]));
            }
        }
        for (shape, color) in &rects {
            eprintln!("{:?} {}", shape, color);
        }

        let mut painter = PainterState::new(&problem);
        let mut moves = vec![];
        let mut root = BlockId::root(0);
        for (shape, color) in &rects {
            let m = PCut { block_id: root.clone(), x: shape.x1 + 1, y: shape.y1 + 1};
            let ids1 = painter.apply_move(&m).new_block_ids;
            moves.push(m);

            let m = PCut { block_id: ids1[2].clone(), x: shape.x2 - 1, y: shape.y2 - 1};
            let ids2 = painter.apply_move(&m).new_block_ids;
            moves.push(m);

            let m = ColorMove { block_id: ids2[0].clone(), color: *color };
            painter.apply_move(&m);
            moves.push(m);

            let m = Merge { block_id1: ids2[0].clone(), block_id2: ids2[1].clone() };
            let qq1 = painter.apply_move(&m).new_block_ids[0].clone();
            moves.push(m);

            let m = Merge { block_id1: ids2[2].clone(), block_id2: ids2[3].clone() };
            let qq2 = painter.apply_move(&m).new_block_ids[0].clone();
            moves.push(m);

            let m = Merge { block_id1: qq2, block_id2: qq1 };
            let qq3 = painter.apply_move(&m).new_block_ids[0].clone();
            moves.push(m);

            let m = Merge { block_id1: ids1[0].clone(), block_id2: ids1[1].clone() };
            let qq4 = painter.apply_move(&m).new_block_ids[0].clone();
            moves.push(m);

            let m = Merge { block_id1: qq3, block_id2: ids1[3].clone() };
            let qq5 = painter.apply_move(&m).new_block_ids[0].clone();
            moves.push(m);

            let m = Merge { block_id1: qq4, block_id2: qq5 };
            let qq6 = painter.apply_move(&m).new_block_ids[0].clone();
            moves.push(m);

            root = qq6;
        }

        eprintln!("---");
        for m in &moves {
            eprintln!("{}", m);
        }
        eprintln!("---");

        painter.render().save(&project_path(format!("outputs/swan_{}_yo.png", problem_id)));
    }
}

// fn isolate_rect(root: &mut BlockId, painter: &mut PainterState) -> Vec<Move> {

// }