#![allow(dead_code)]
#![allow(clippy::needless_range_loop)]

use std::collections::HashMap;
use rand::prelude::*;
use rstats::{VecVec};

use crate::{basic::{Color, Shape }, image::Image, util::project_path};

fn color_to_f64(c: &Color) -> [f64; 4] {
    [c.0[0] as f64, c.0[1] as f64, c.0[2] as f64, c.0[3] as f64]
}

fn f64_to_color(c: &[f64; 4]) -> Color {
    Color([c[0].round() as u8, c[1].round() as u8, c[2].round() as u8, c[3].round() as u8])
}

fn shape_distance(colors: &HashMap<Color, f64>, color: &Color) -> f64 {
    let mut d = 0f64;
    for (k, v) in colors {
        d += color.dist(k) * v;
    }
    d
}

static K_STEP_NUM: i32 = 500;
static K_STEP: f64 = 0.8;

fn gradient_step(colors: &HashMap<Color, f64>, color: &mut [f64; 4], step_size: f64) {
    let mut gradient = [0f64; 4];
    for (k, v) in colors {
        let mut step = [0f64; 4];
        let mut len = 0f64;
        for i in 0..4 {
            step[i] = k.0[i] as f64 - color[i] as f64;
            len += step[i] * step[i];
        }
        if len > 1.0 {
            let coeff = 1.0 / len.sqrt() * v;
            for i in 0..4 {
                gradient[i] += step[i] * coeff;
            }
        }
    }
    let mut len = 0f64;
    for i in 0..4 { len += gradient[i] * gradient[i]; }
    if len > 1.0 {
        for i in 0..4 { color[i] += gradient[i] / len.sqrt() * step_size; }
    }
}

pub fn dist_to_color_freqs(color_freqs: &HashMap<Color, f64>, color: &Color) -> f64 {
    let mut res = 0f64;
    for (k, v) in color_freqs {
        res += color.dist(k) * v;
    }
    res * 0.005
}

pub fn color_freqs(pic: &Image, shape: &Shape) -> HashMap<Color, f64> {
    let mut colors: HashMap<Color, f64> = HashMap::new();
    for x in shape.x1..shape.x2 {
        for y in shape.y1..shape.y2 {
            *colors.entry(pic.get_pixel(x, y)).or_default() += 1.0;
        }
    }
    colors
}

fn optimal_color_for_color_freqs(color_freqs: &HashMap<Color, f64>) -> Color {
    let color = Color::default();
    let mut color_array = color_to_f64(&color);
    let mut step_size: f64 = 100.0;
    let mut last_color = color_array;
    let mut last_dist = dist_to_color_freqs(color_freqs, &f64_to_color(&last_color));
    for _ in 0..K_STEP_NUM {
        gradient_step(color_freqs, &mut color_array, step_size);
        let new_dist = dist_to_color_freqs(color_freqs, &f64_to_color(&color_array));
        if step_size == 1.0 && new_dist.abs() > last_dist.abs() { break; }
        last_dist = new_dist;
        last_color = color_array;

        step_size *= K_STEP;
        if step_size < 1.0 { step_size = 1.0; }
    }
    f64_to_color(&last_color)
}

// Gradient descent based. See also gmedian_color().
pub fn optimal_color_for_block(pic: &Image, shape: &Shape) -> Color {
    let colors = color_freqs(pic, shape);
    optimal_color_for_color_freqs(&colors)
}

pub fn best_from_palette(color_freqs: &HashMap<Color, f64>, palette: &[Color]) -> usize {
    (0..palette.len()).min_by_key(
        |&i| (1000.0 * dist_to_color_freqs(color_freqs, &palette[i])).round() as i64,
    ).unwrap()
}

pub fn k_means(color_freqss: &[HashMap<Color, f64>], num_clusters: usize) -> Vec<Color> {
    let _t = crate::stats_timer!("k_means").time_it();
    let mut centers = vec![];
    for _ in 0..num_clusters {
        centers.push(Color([
            thread_rng().gen(),
            thread_rng().gen(),
            thread_rng().gen(),
            thread_rng().gen(),
        ]));
    }

    for _ in 0..50 {
        let mut cluster_freqss: Vec<HashMap<Color, f64>> = vec![HashMap::new(); num_clusters];
        for freqs in color_freqss {
            let nearest = best_from_palette(freqs, &centers);
            for (k, v) in freqs {
                *cluster_freqss[nearest].entry(*k).or_default() += v;
            }
        }
        for (i, cf) in cluster_freqss.iter().enumerate() {
            if cf.is_empty() {
                centers[i] = Color([
                    thread_rng().gen(),
                    thread_rng().gen(),
                    thread_rng().gen(),
                    thread_rng().gen(),
                ]);
            } else {
                centers[i] = optimal_color_for_color_freqs(cf);
            }
        }
    }
    centers
}

crate::entry_point!("gradient", gradient);
fn gradient() {
    let args: Vec<String> = std::env::args().skip(2).collect();
    if args.len() != 1 {
        println!("Usage: cargo run --release gradient <problem id>");
        std::process::exit(1);
    }
    let problem_id: i32 = args[0].parse().unwrap();

    let target = Image::load(&project_path(format!("data/problems/{}.png", problem_id)));

    let mut colors: HashMap<Color, f64> = HashMap::new();
    for x in 0..target.width {
        for y in 0..target.height {
            *colors.entry(target.get_pixel(x, y)).or_default() += 1.0;
        }
    }
    let shape = Shape{x1: 0, x2: target.width, y1: 0, y2: target.height};

    let start = std::time::Instant::now();
    let colors = color_freqs(&target, &shape);
    dbg!(start.elapsed());

    let start = std::time::Instant::now();
    let res = optimal_color_for_color_freqs(&colors);
    dbg!(start.elapsed());
    println!("{}", shape_distance(&colors, &res));
}

pub fn mean_color(img: &Image, shape: Shape) -> Color {
    let (mut r, mut g, mut b, mut a) = (0u32, 0u32, 0u32, 0u32);
    let mut pixels = 0;
    for x in shape.x1..shape.x2 {
        for y in shape.y1..shape.y2 {
            let p = img.get_pixel(x, y);
            r += p.0[0] as u32; g += p.0[1] as u32; b += p.0[2] as u32; a += p.0[3] as u32;
            pixels += 1;
        }
    }
    Color([
        (r / pixels) as u8,
        (g / pixels) as u8,
        (b / pixels) as u8,
        (a / pixels) as u8,
    ])
}

// Geometric median calculated using Weiszfeld's algorithm.
// Probably better than gradient descent.
pub fn gmedian_color(img: &Image, shape: Shape) -> Color {
    let mut colors = vec![];
    for y in shape.y1..shape.y2 {
        for x in shape.x1..shape.x2 {
            colors.push(Vec::from(img.get_pixel(x, y).0));
        }
    }
    let median = colors.gmedian(0.5);
    for component in median.iter() {
        if component.is_nan() {
            // Fallback to mean. I've no idea why the median algorithm fails sometimes.
            return mean_color(img,shape);
        }
    }
    Color ([
        median[0].round() as u8,
        median[1].round() as u8,
        median[2].round() as u8,
        median[3].round() as u8,
    ])
}