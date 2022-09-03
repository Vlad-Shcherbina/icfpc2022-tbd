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
    let mut q = 0;
    let mut target = Image::new(target.width, target.height, Color::default());
    for (color, cnt) in cnts {
        for _ in 0..cnt {
            target.set_pixel(q % target.width, q / target.width, color);
            q += 1;
        }
    }

    let mut rects: Vec<(Shape, Color)> = vec![];
    for i in 0..target.height {
        let mut color: Option<Color> = None;
        let mut len = 0;
        for j in 0..target.width {
            let c = target.get_pixel(j, i);
            if Some(c) == color {
                len += 1;
            } else {
                if len > 0 {
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
        if len > 0 {
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
    let target = random_image(10, 15);
    let stuff = packing(&target);
    let image = reconstruct(&target, &stuff);
    let d = dist(&target, &image);
    dbg!(d);
}
