#![allow(dead_code)]
#![allow(clippy::needless_range_loop)]

use std::collections::HashMap;
use rand::prelude::*;

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

fn gradient_step(colors: &HashMap<Color, f64>, color: &mut [f64; 4]) {
    let mut gradient = [0f64; 4];
    for (k, v) in colors {
        let mut step = [0f64; 4];
        let mut len = 0f64;
        for i in 0..4 {
            step[i] = k.0[i] as f64 - color[i] as f64;
            len += step[i] * step[i];
        }
        if len > 1.0 {
            for i in 0..4 {
                gradient[i] += step[i] / len.sqrt() * v;
            }
        }
    }
    let mut len = 0f64;
    for i in 0..4 { len += gradient[i] * gradient[i]; }
    if len > 1.0 {
        for i in 0..4 { color[i] += gradient[i] / len.sqrt(); }
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
    let k_step_num = 600;
    let color = Color::default();
    let mut color_array = color_to_f64(&color);
    for _ in 0..k_step_num {
        gradient_step(color_freqs, &mut color_array);
    }
    f64_to_color(&color_array)
}

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
    let target = Image::load(&project_path(format!("data/problems/{}.png", 11)));

    let mut colors: HashMap<Color, f64> = HashMap::new();
    for x in 0..target.width {
        for y in 0..target.height {
            *colors.entry(target.get_pixel(x, y)).or_default() += 1.0;
        }
    }

    let mut color_array = [0f64; 4];
    let mut total = 0f64;

    for (k, v) in &colors {
        for i in 0..4 {
            color_array[i] += k.0[i] as f64;
        }
        total += v;
    }

    for i in 0..4 { color_array[i] /= (target.height * target.width) as f64; }
    println!("{}", shape_distance(&colors, &f64_to_color(&color_array)));
    println!("{}", shape_distance(&colors, &optimal_color_for_block(&target,
        &Shape{x1: 0, x2: target.width, y1: 0, y2: target.height})));
}
