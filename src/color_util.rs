#![allow(dead_code)]
#![allow(unused_imports)]
#![allow(clippy::needless_range_loop)]

use rand::prelude::*;
use rand::prelude::*;
use std::collections::{HashMap, HashSet};

use crate::basic::*;
use crate::image::Image;

fn color_freqs_distance(colors: &HashMap<Color, f64>, color: Color) -> f64 {
    let mut d = 0f64;
    for (k, v) in colors {
        d += color.dist(k) * v;
    }
    d
}

pub fn dist_to_color_freqs(color_freqs: &HashMap<Color, f64>, color: Color) -> f64 {
    let mut res = 0f64;
    for (k, v) in color_freqs {
        res += color.dist(k) * v;
    }
    res * 0.005
}

pub fn color_freqs(pic: &Image, shape: &Shape) -> HashMap<Color, f64> {
    let mut colors: HashMap<Color, f64> = HashMap::default();
    for x in shape.x1..shape.x2 {
        for y in shape.y1..shape.y2 {
            *colors.entry(pic.get_pixel(x, y)).or_default() += 1.0;
        }
    }
    colors
}

// The multivariate L1-median and associated data depth
// Yehuda Vardi and Cun-Hui Zhang
// https://www.ncbi.nlm.nih.gov/pmc/articles/PMC26449/pdf/pq001423.pdf
fn modified_weiszfeld(color_freqs: &HashMap<Color, f64>, iterations: i32) -> Color {
    let _t = crate::stats_timer!("modified_weiszfeld").time_it();
    if color_freqs.len() <= 2 {
        // Because their algorithm doesn't work if all colors are collinear.
        let mut best_freq = -1.0;
        let mut best_color = Color([0; 4]);
        for (&color, &freq) in color_freqs {
            if freq > best_freq {
                best_freq = freq;
                best_color = color;
            }
        }
        return best_color;
    }

    let color_freqs_vec: Vec<(Color, f64)> = color_freqs.iter().map(|(k, v)| (*k, *v)).collect();
    let mut weights = vec![0f64; color_freqs_vec.len()];
    let mut y = [128.0f64; 4];
    for _ in 0..iterations {
        crate::stats_counter!("modified_weiszfeld/iteration").inc();
        let mut collision = None;
        for (i, &(c, freq)) in color_freqs_vec.iter().enumerate() {
            let mut d = 0f64;
            for j in 0..4 {
                let t = c.0[j] as f64 - y[j];
                d += t * t;
            }
            if d < 1e-30 {
                assert!(collision.is_none());
                collision = Some(i);
                weights[i] = 0.0;
            } else {
                weights[i] = freq / d.sqrt();
                assert!(weights[i].is_finite(), "{:?} {:?}", freq, d);
            }
        }
        let sum = weights.iter().sum::<f64>();
        let inv_sum = 1.0 / sum;
        assert!(inv_sum.is_finite());
        for w in &mut weights {
            *w *= inv_sum;
        }
        y = [0f64; 4];
        for (i, &(c, _)) in color_freqs_vec.iter().enumerate() {
            for j in 0..4 {
                y[j] += c.0[j] as f64 * weights[i];
            }
        }

        if let Some(collision) = collision {
            let mut rs = [0f64; 4];
            for (i, &(c, freq)) in color_freqs_vec.iter().enumerate() {
                if i == collision {
                    continue;
                }
                let mut d = 0f64;
                for j in 0..4 {
                    let t = c.0[j] as f64 - y[j];
                    d += t * t;
                }
                if d <= 1e-30 {
                    continue; // hack
                }
                assert!(d > 1e-30);
                let dd = 1.0 / d.sqrt();
                assert!(d.is_finite());
                for j in 0..4 {
                    rs[j] += freq * (c.0[j] as f64 - y[j]) * dd;
                }
            }
            let mut r = 0f64;
            for j in 0..4 {
                r += rs[j] * rs[j];
            }
            let r = r.sqrt();
            if r > 1e-30 {
                // hack
                assert!(r > 1e-30);
                let freq = color_freqs_vec[collision].1;
                let alpha = (freq / r).min(1.0);
                assert!(alpha.is_finite());

                let c = color_freqs_vec[collision].0;
                for j in 0..4 {
                    y[j] = (1.0 - alpha) * y[j] + alpha * c.0[j] as f64;
                }
            }
        }

        for i in 0..4 {
            assert!(y[i].is_finite(), "{:?} {:?}", y, weights);
        }
        // dbg!(f64_to_color(&y));
    }
    // dbg!(y);

    let mut best_dist = 1e20;
    let mut best_color = Color::default();
    // try to round each component both up and down
    for mask in 0..16 {
        let c = Color([
            (y[0] as u8).wrapping_add(mask & 1),
            (y[1] as u8).wrapping_add((mask >> 1) & 1),
            (y[2] as u8).wrapping_add((mask >> 2) & 1),
            (y[3] as u8).wrapping_add(mask >> 3),
        ]);
        let dist = color_freqs_distance(color_freqs, c);
        if dist < best_dist {
            best_dist = dist;
            best_color = c;
        }
    }
    best_color
}

fn climb(color_freqs: &HashMap<Color, f64>, mut color: Color) -> Color {
    let _t = crate::stats_timer!("climb").time_it();
    loop {
        crate::stats_counter!("climb/iteration").inc();
        let mut d = color_freqs_distance(color_freqs, color);
        let mut changed = false;
        for i in 0..4 {
            for delta in [-1, 1] {
                let mut c2 = color;
                c2.0[i] = (c2.0[i] as i32 + delta) as u8;
                let d2 = color_freqs_distance(color_freqs, c2);
                if d2 < d {
                    color = c2;
                    d = d2;
                    changed = true;
                }
            }
        }
        if !changed {
            return color;
        }
    }
}

pub fn optimal_color_for_color_freqs(color_freqs: &HashMap<Color, f64>) -> Color {
    let color = modified_weiszfeld(color_freqs, 10);
    climb(color_freqs, color)
}

fn check_optimal_color_for_color_freqs(color_freqs: &HashMap<Color, f64>, color: Color) {
    let c2 = climb(color_freqs, color);
    assert_eq!(c2, color);
    // assert!(c2.dist(&color) < 10.0, "{} {}", c2, color);
}

// Gradient descent based. See also gmedian_color().
pub fn optimal_color_for_block(pic: &Image, shape: &Shape) -> Color {
    let colors = color_freqs(pic, shape);
    optimal_color_for_color_freqs(&colors)
}

pub fn best_from_palette(color_freqs: &HashMap<Color, f64>, palette: &[Color]) -> usize {
    (0..palette.len())
        .min_by_key(|&i| (1000.0 * dist_to_color_freqs(color_freqs, palette[i])).round() as i64)
        .unwrap()
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
        let mut cluster_freqss: Vec<HashMap<Color, f64>> = vec![HashMap::default(); num_clusters];
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

#[test]
fn test_modified_weiszfeld() {
    let mut rng = rand::rngs::StdRng::seed_from_u64(42);
    for _ in 0..1000 {
        let mut color_freqs: HashMap<Color, f64> = HashMap::default();
        for _ in 0..rng.gen_range(1..10) {
            let c = Color([rng.gen(), rng.gen(), rng.gen(), rng.gen()]);
            let freq = rng.gen_range(1..1000) as f64;
            *color_freqs.entry(c).or_default() += freq;
        }
        dbg!(&color_freqs);
        let color = optimal_color_for_color_freqs(&color_freqs);
        check_optimal_color_for_color_freqs(&color_freqs, color);
    }
}

crate::entry_point!("gradient", gradient);
fn gradient() {
    /*let mut color_freqs = HashMap::default();
    color_freqs.insert(Color([73, 12, 25, 19]), 30.0);
    color_freqs.insert(Color([201, 25, 148, 55]), 846.0);
    color_freqs.insert(Color([227, 119, 86, 143]), 146.0);

    let color = modified_weiszfeld(&color_freqs, 20);
    dbg!(color);
    check_optimal_color_for_color_freqs(&color_freqs, color);*/

    let start = std::time::Instant::now();
    for problem_id in 1..=35 {
        // dbg!(problem_id);
        eprintln!("--------- {} ---------", problem_id);
        let problem = Problem::load(problem_id);

        let color_freqs = color_freqs(&problem.target, &Shape::from_image(&problem.target));
        // let color = optimal_color_for_color_freqs(&color_freqs);
        // eprintln!("{} {}", color, dist_to_color_freqs(&color_freqs, color));
        let color = optimal_color_for_color_freqs(&color_freqs);
        // dbg!(color);
        check_optimal_color_for_color_freqs(&color_freqs, color);
    }
    eprintln!("it took {:?}", start.elapsed());
    eprintln!("{}", crate::stats::STATS.render());
}

pub fn mean_color(img: &Image, shape: Shape) -> Color {
    let (mut r, mut g, mut b, mut a) = (0u32, 0u32, 0u32, 0u32);
    let mut pixels = 0;
    for x in shape.x1..shape.x2 {
        for y in shape.y1..shape.y2 {
            let p = img.get_pixel(x, y);
            r += p.0[0] as u32;
            g += p.0[1] as u32;
            b += p.0[2] as u32;
            a += p.0[3] as u32;
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

pub fn adjust_colors(problem: &Problem, moves: &Vec<Move>) -> Vec<Move> {
    let mut painter = PainterState::new(problem);
    let mut color_moves = HashSet::new();
    for mv in moves {
        painter.apply_move(mv);
        if let Move::ColorMove { block_id: _, color } = mv {
            color_moves.insert(color);
        }
    }
    let mut freqs: HashMap<Color, HashMap<Color, f64>> = color_moves
        .into_iter()
        .map(|color| (*color, HashMap::new()))
        .collect();
    let img = painter.render();
    for x in 0..img.width {
        for y in 0..img.height {
            let color = img.get_pixel(x, y);
            if let Some(freq) = freqs.get_mut(&color) {
                *freq.entry(problem.target.get_pixel(x, y)).or_default() += 1.0;
            }
        }
    }
    let new_colors: HashMap<Color, Color> = freqs
        .into_iter()
        .map(|(color, freq)| (color, optimal_color_for_color_freqs(&freq)))
        .collect();
    moves
        .iter()
        .map(|mv| match mv {
            Move::ColorMove { block_id, color } => Move::ColorMove {
                block_id: block_id.clone(),
                color: new_colors[color],
            },
            _ => mv.clone(),
        })
        .collect()
}
