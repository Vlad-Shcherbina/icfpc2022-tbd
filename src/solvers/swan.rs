use crate::util::project_path;
use crate::basic::*;
use crate::image::Image;
// use crate::invocation::{record_this_invocation, Status};
// use crate::uploader::upload_solution;
use crate::solver_utils::*;

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
        let target = Image::load(&project_path(format!("data/problems/{}.png", problem_id)));

        let px = 40;
        let py = 40;

        let w = target.width / px;
        let h = target.height / py;

        let mut color_freqss = vec![];
        for i in 0..h {
            for j in 0..w {
                let color_freqs = color_freqs(&target, &Shape {
                    x1: j * px,
                    y1: i * py,
                    x2: (j + 1) * px,
                    y2: (i + 1) * py,
                });
                color_freqss.push(color_freqs);
            }
        }

        let palette = k_means(&color_freqss, 3);

        let mut approx_target = Image::new(target.width, target.height, Color::default());
        for i in 0..h {
            for j in 0..w {
                let shape = Shape {
                    x1: j * px,
                    y1: i * py,
                    x2: (j + 1) * px,
                    y2: (i + 1) * py,
                };
                let color_freqs = color_freqs(&target, &shape);
                let color = palette[best_from_palette(&color_freqs, &palette)];
                approx_target.fill_rect(shape, color);
            }
        }
        approx_target.save(&project_path(format!("outputs/swan_{}_approx.png", problem_id)));
    }
}
