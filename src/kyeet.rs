use std::{cmp::Ordering, path::Path};
use fxhash::FxHashMap as HashMap;

use crate::{
    basic::{Color, Problem, Shape},
    color_util::{closest_color_from_colors, color_freqs, optimal_color_for_color_freqs},
    image::Image,
};

crate::entry_point!("kyeet", kyeet);
fn kyeet() {
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
    dbg!(&dry_run);

    for problem_id in problem_range {
        let problem = Problem::load(problem_id);
        for dt in [
            10.0, 20.0, 30.0, 40.0, 50.0, 75.0, 100.0, 110.0, 120.0, 130.0, 140.0, 150.0, 160.0,
            170.0, 180.0, 190.0, 200.0, 210.0, 220.0, 230.0, 240.0, 250.0, 260.0, 270.0, 280.0,
            290.0, 300.0, 400.0,
        ] {
            // for dt in [220.0] {
            // let dt = 250.0;
            let res = kyeet_do(&KyeetArgs { dt }, &problem);
            res.save(Path::new(&format!(
                "data/kyeet/{}/{:?}.png",
                problem_id, dt
            )));
        }
    }
}

pub struct KyeetArgs {
    dt: f64,
}

// pub fn portrait_of_initial_image_as_a_young_target_image(
//     args: &KyeetArgs,
//     problem: &Problem,
// ) -> Vec<Color> {
//     kyeet_do(args, problem)
//     vec![]
// }

/*

initial_colors = [...]
target_colors = [...]

if target_colors.len >= initial_colors.len {
    for c in initial_colors {
        target_colors.min_by_key(|x| c.dist(x)) -> c
    }
} else ???

TARGET PALETTE: 0 2 3 10
INITIAL PALETTE: 0 3.5 4 12
RES: 0 2 4 12
NORM: 0 4 3.5 12

=== PROBLEMATIC: ===
TARGET PALETTE: 0 1 2
INITIAL PALETTE: 0 1
SHOULDBE: 0 1 2 (id)
RES: 0 1 1
:(

*/

pub fn kyeet_do(args: &KyeetArgs, problem: &Problem) -> Image {
    let beep = squeeze_color_range(args, problem);
    let res_palette = match beep.len() {
        2 => match problem.id {
            Some(n) => {
                if n > 25 && n < 36 {
                    eprintln!("!!!!");
                    let arm1 =
                        tweak_target_palette_for_26_35(beep.get(1).unwrap(), beep.first().unwrap());
                    arm1
                } else {
                    let arm2 = beep.get(1).unwrap().clone();
                    arm2
                }
            }
            None => {
                let arm3 = beep.get(1).unwrap().clone();
                arm3
            }
        },
        _otherwise => beep.first().unwrap().clone(),
    };

    let mut res = problem.target.clone();

    for ix in 0..problem.target.width {
        for iy in 0..problem.target.height {
            let c = problem.target.get_pixel(ix, iy);
            // TODO: consider weights?!
            res.set_pixel(ix, iy, closest_color_from_colors(&c, &res_palette).1);
        }
    }

    res
}

pub fn tweak_target_palette_for_26_35(
    target_palette: &Vec<Color>,
    initial_palette: &Vec<Color>,
) -> Vec<Color> {
    // eprintln!("=======================================");
    let mut tp1 = target_palette.clone();
    dbg!(target_palette.clone());
    dbg!(initial_palette.clone());
    for ic in initial_palette {
        let (_d, c) = closest_color_from_colors(ic, target_palette);
        let mut tp2: Vec<Color> = vec![];
        for x in tp1.clone() {
            if initial_palette.contains(&x) {
                // eprintln!("<><><><><><> hoyo <>< ><><><><><><><><><>` {:?}", x);
                continue;
            }
            if x == c {
                // eprintln!("|||||||||||| KAPPA ||||||||||| {:?}", x);
                tp2.push(*ic);
            } else {
                // eprintln!("=-=-=-=-=- DELTA -=-=-=-=-= {:?}", x);
                tp2.push(x);
            }
        }
        tp1 = tp2.clone();
        // eprintln!("=== yoyoba ===");
    }
    dbg!(tp1.clone());
    tp1
}

pub fn squeeze_color_range(args: &KyeetArgs, problem: &Problem) -> Vec<Vec<Color>> {
    // let scrape_colors_from: Image = problem
    //     .clone()
    //     .initial_img
    //     .unwrap_or_else(|| problem.target.clone());

    let scrapes = match problem.clone().initial_img {
        None => vec![problem.target.clone()],
        Some(x) => vec![x, problem.target.clone()],
    };

    // let scrape_colors_from: Image = problem.clone().initial_img.unwrap();
    // TODO: we can remember the frequencies of each colour!!!
    scrapes
        .iter()
        .map(|scrape_colors_from| {
            let cfs = color_freqs(
                scrape_colors_from,
                &Shape {
                    x1: 0,
                    y1: 0,
                    x2: scrape_colors_from.width,
                    y2: scrape_colors_from.height,
                },
            );

            let mut centered: HashMap<Color, f64> = HashMap::default();
            let mut cfs1 = cfs;
            while !cfs1.is_empty() {
                let (tube, cfs2) = squeeze_once(args.dt, &cfs1, false);
                cfs1 = cfs2;
                centered.insert(tube.0, tube.1);
            }

            let colors = centered.keys();
            let mut colors_vec = vec![];

            for ccc in colors {
                colors_vec.push(*ccc);
            }

            colors_vec.clone()
        })
        .collect()
}

fn index_cfs(cfs: HashMap<Color, f64>) -> (Vec<(f64, Color)>, Vec<Color>) {
    let mut vs: Vec<(f64, Color)> = cfs.iter().map(|(c, v)| (*v, *c)).collect();
    vs.sort_by(|a, b| a.clone().partial_cmp(b).unwrap());
    // most popular colour
    let mpc = vs.first().unwrap().1;
    let mut css: Vec<Color> = cfs.keys().map(|x| Color(x.0)).collect();
    css.sort_by(|a, b| {
        if mpc.clone().dist(a) < mpc.clone().dist(b) {
            Ordering::Less
        } else if mpc.clone().dist(a) == mpc.clone().dist(b) {
            Ordering::Equal
        } else {
            Ordering::Greater
        }
    });

    (vs.clone(), css.clone())
}

fn squeeze_once(
    dt: f64,
    cfs: &HashMap<Color, f64>,
    debug: bool,
) -> ((Color, f64), HashMap<Color, f64>) {
    let mut cfs1 = cfs.clone();

    if debug {
        dbg!(cfs1.len());
    }

    let (vs, css) = index_cfs(cfs.clone());

    let vs0 = *vs.first().unwrap();

    let mut last_freqs: HashMap<Color, f64> = HashMap::default();

    last_freqs.insert(vs0.1, vs0.0);

    for c in css {
        let d = vs0.1.dist(&c);
        if d < dt {
            let c_freq = *cfs.get(&c).unwrap();
            last_freqs.insert(c, c_freq);
            cfs1.remove_entry(&c);
        } else {
            break;
        }
    }

    let rep = optimal_color_for_color_freqs(&last_freqs);
    let mut rep_freq = 0.0;
    for x in last_freqs.clone() {
        rep_freq += x.1;
    }

    if debug {
        dbg!(cfs1.len());
        eprintln!("* * *");
    }

    ((rep, rep_freq), cfs1.clone())
}
