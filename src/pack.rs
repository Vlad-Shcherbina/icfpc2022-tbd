use std::collections::HashMap;
use rand::prelude::*;
use crate::image::Image;
use crate::basic::{Shape, Color};

pub fn packing(target: &Image) -> Vec<(Shape, Color)> {
    let mut cnts: HashMap<Color, i32> = HashMap::new();
    for y in 0..target.height {
        for x in 0..target.width {
            *cnts.entry(target.get_pixel(x as i32, y as i32)).or_default() += 1;
        }
    }
    let bg_color = *cnts.iter().max_by_key(|(_, &cnt)| cnt).unwrap().0;
    let mut q = 0;
    let mut target = Image::new(target.width, target.height, Color::default());
    for (color, cnt) in cnts {
        for _ in 0..cnt {
            target.set_pixel(q % target.width, q / target.width, color);
            q += 1;
        }
    }

    let mut rects: Vec<(Shape, Color)> = vec![];
    rects.push((Shape::from_image(&target), bg_color));

    for i in 0..target.height {
        let mut color: Option<Color> = None;
        let mut len = 0;
        for j in 0..target.width {
            let c = target.get_pixel(j, i);
            if Some(c) == color {
                len += 1;
            } else {
                if len > 0 && color.unwrap() != bg_color {
                    rects.push((Shape {
                        x1: j - len,
                        y1: i,
                        x2: j,
                        y2: i + 1,
                    }, color.unwrap()));
                }
                color = Some(c);
                len = 1;
            }
        }
        if len > 0 && color.unwrap() != bg_color {
            let j = target.width;
            rects.push((Shape {
                x1: j - len,
                y1: i,
                x2: j,
                y2: i + 1,
            }, color.unwrap()));
        }
    }
    rects
}

struct State {
    target: Image,
    img: Image,
    nac: Color,
    rects: Vec<(Shape, Color)>,
    covered: i32,
    cnts: Vec<(Color, i32)>,
    solution: Option<Vec<(Shape, Color)>>,
    best_cost: i32,
    cost: i32,
}

fn rec(i: usize, state: &mut State) {
    if i + 1 == state.cnts.len() {
        let mut sol = state.rects.clone();
        sol.push((Shape::from_image(&state.img), state.cnts[i].0));
        sol.reverse();
        let mut cost = 0;
        for y in 0..state.target.height {
            for x in 0..state.target.width {
                let mut c = state.img.get_pixel(x, y);
                if c == state.nac {
                    c = state.cnts[i].0;
                }
                if c != state.target.get_pixel(x, y) {
                    cost += 1;
                }
            }
        }
        if cost < state.best_cost {
            state.best_cost = cost;
            state.solution = Some(sol);
        }
        return;
    }
    // if state.solution.is_some() {
    //     return
    // }
    for w in 1..state.target.width + 1 {
        for h in 1..state.target.height + 1 {
            if w * h < state.cnts[i].1 {
                continue;
            }
            if w * h > state.cnts[i].1 + state.covered {
                continue;
            }
            for y1 in 0..state.target.height + 1 - h {
                for x1 in 0..state.target.width + 1 - w {
                    let mut covered_inside = 0;
                    for y in y1..y1 + h {
                        for x in x1..x1 + w {
                            if state.img.get_pixel(x, y) != state.nac {
                                covered_inside += 1;
                            }
                        }
                    }
                    if state.cnts[i].1 + covered_inside != w * h {
                        continue;
                    }
                    // eprintln!("  {}{:?}", "  ".repeat(i as usize), (x1, y1, w, h));

                    let old_img = state.img.clone();
                    let old_cost = state.cost;

                    for y in y1..y1 + h {
                        for x in x1..x1 + w {
                            if state.img.get_pixel(x, y) == state.nac {
                                state.img.set_pixel(x, y, state.cnts[i].0);
                                if state.target.get_pixel(x, y) != state.cnts[i].0 {
                                    state.cost += 1;
                                }
                            }
                        }
                    }

                    state.rects.push((Shape {
                        x1,
                        y1,
                        x2: x1 + w,
                        y2: y1 + h,
                    }, state.cnts[i].0));

                    if state.cost < state.best_cost {
                        rec(i + 1, state);
                    }

                    state.rects.pop().unwrap();
                    state.img = old_img;
                    state.cost = old_cost;
                }
            }
        }
    }
}

pub fn packing2(target: &Image) -> Vec<(Shape, Color)> {
    let _t = crate::stats_timer!("packing2").time_it();
    let mut cnts: HashMap<Color, i32> = HashMap::new();
    for y in 0..target.height {
        for x in 0..target.width {
            *cnts.entry(target.get_pixel(x as i32, y as i32)).or_default() += 1;
        }
    }
    let nac = (0..255).map(|i| Color([i, i, i, i])).find(|c| !cnts.contains_key(c)).unwrap();
    // dbg!(nac);
    let mut cnts: Vec<_> = cnts.into_iter().collect();
    cnts.sort_by_key(|&(_, cnt)| cnt);
    // dbg!(&cnts);

    let mut state = State {
        target: target.clone(),
        img: Image::new(target.width, target.height, nac),
        nac,
        rects: vec![],
        covered: 0,
        cnts,
        solution: None,
        best_cost: i32::MAX,
        cost: 0,
    };
    rec(0, &mut state);

    match state.solution {
        Some(sol) => sol,
        None => {
            eprintln!("FALLBACK");
            packing(target)
        }
    }
}


fn reconstruct(target: &Image, stuff: &[(Shape, Color)]) -> Image {
    let mut image = Image::new(target.width, target.height, Color([42; 4]));
    for (shape, color) in stuff {
        image.fill_rect(*shape, *color);
    }
    image
}

fn random_image(width: i32, height: i32) -> Image {
    let num_colors = thread_rng().gen_range(1..7);
    let palette: Vec<Color> = (0..num_colors)
    .map(|_| Color([thread_rng().gen(), thread_rng().gen(), thread_rng().gen(), thread_rng().gen()]))
    .collect();

    let mut img = Image::new(width, height, Color([0; 4]));
    for y in 0..height {
        for x in 0..width {
            img.set_pixel(x, y, palette[thread_rng().gen_range(0..num_colors)]);
        }
    }
    img
}

fn dist(img1: &Image, img2: &Image) -> i32 {
    let mut d = 0;
    assert_eq!(img1.width, img2.width);
    assert_eq!(img1.height, img2.height);
    let mut colors1 = vec![];
    let mut colors2 = vec![];
    for y in 0..img1.height {
        for x in 0..img1.width {
            let c1 = img1.get_pixel(x, y);
            let c2 = img2.get_pixel(x, y);
            if c1 != c2 {
                d += 1;
            }
            colors1.push(c1);
            colors2.push(c2);
        }
    }
    colors1.sort();
    colors2.sort();
    assert_eq!(colors1, colors2);
    d
}

crate::entry_point!("pack_demo", pack_demo);
fn pack_demo() {
    let target = random_image(10, 10);
    let stuff = packing2(&target);
    eprintln!("{:?}", stuff);
    let image = reconstruct(&target, &stuff);
    let d = dist(&target, &image);
    dbg!(d);
}
