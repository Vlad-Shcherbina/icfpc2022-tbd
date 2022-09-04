use std::{cmp::Ordering, collections::HashMap, hash::BuildHasherDefault, vec};

use fxhash::FxHasher;

use crate::{
    basic::{Color, Problem, Shape},
    color_util::{best_from_palette, color_freqs, optimal_color_for_color_freqs},
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
        kyeet_do(&KyeetArgs { dt: 100.0 }, &problem);
    }
}

struct KyeetArgs {
    dt: f64,
}

fn kyeet_do(args: &KyeetArgs, problem: &Problem) -> Image {
    let scrape_colors_from: Image = problem
        .clone()
        .initial_img
        .unwrap_or(problem.target.clone());

    let cfs = color_freqs(
        &scrape_colors_from,
        &Shape {
            x1: 0,
            y1: 0,
            x2: scrape_colors_from.width,
            y2: scrape_colors_from.height,
        },
    );

    let mut centered: HashMap<Color, f64> = HashMap::default();
    let mut cfs1 = cfs.clone();
    // let mut cfs1_ptr = &cfs1;
    let mut i = 0;
    eprintln!("! ({}) !", cfs1.len());
    while cfs1.len() > 0 {
        // eprintln!("! ({}) !", cfs1.len());
        i += 1;
        if i % 100 == 0 {
            eprint!("({})", cfs1.len());
        }
        // let cfs2 = cfs1.clone();
        // let tube_r = cfs2.iter().next().unwrap();
        // let tube = (tube_r.0.clone(), tube_r.1.clone());
        let (tube, cfs2) = squeeze_once(args.dt, &cfs1, i % 100 == 0);
        cfs1.retain(|_, _| false);
        for c2 in cfs2 {
            cfs1.insert(c2.0, c2.1);
        }
        centered.insert(tube.0, tube.1);
    }

    dbg!("Yoba {}", centered);

    Image::new(1, 1, Color([255, 255, 255, 255]))
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
