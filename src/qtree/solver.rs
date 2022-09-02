use std::collections::HashSet;
use std::fs::File;
use std::path::PathBuf;
use image::{Rgba, RgbaImage};
use crate::basic::{block_id_to_string, Color, Move, PainterState, Shape};
use crate::basic::Move::PCut;
use crate::util::project_path;
use std::io::Write;
crate::entry_point!("qtree", qtree);

fn read_img(path: PathBuf) -> image::RgbaImage {
    let img = image::open(path).unwrap();
    img.to_rgba8()
}

struct State {
    img: RgbaImage,
    moves: Vec<Move>,
}

impl State {
    fn new(img: RgbaImage) -> State {
        State {
            img,
            moves: vec![],
        }
    }

    fn get_pixel(&self, x: u32, y: u32) -> &Rgba<u8> {
        self.img.get_pixel(x, self.img.height() - y - 1)
    }

    fn solve(&mut self) {
        let shape = Shape {x1: 0, y1: 0, x2: self.img.width() as i32, y2: self.img.height() as i32 };
        let block = vec![0];
        self.qtree(shape, block)
    }

    fn average_color(&self, shape: Shape) -> Color {
        let (mut r, mut g, mut b, mut a) = (0u32, 0u32, 0u32, 0u32);
        let mut pixels = 0;
        for x in shape.x1..shape.x2 {
            for y in shape.y1..shape.y2 {
                let p = self.get_pixel(x as u32, y as u32);
                r += p.0[0] as u32; g += p.0[1] as u32; b += p.0[2] as u32; a += p.0[3] as u32;
                pixels += 1;
            }
        }
        Color {
            r: (r / pixels) as u8,
            g: (g / pixels) as u8,
            b: (b / pixels) as u8,
            a: (a / pixels) as u8,
        }
    }

    fn is_uniform_color(&self, shape: Shape) -> bool {
        let mut colors = HashSet::<Rgba<u8>>::new();
        for x in shape.x1..shape.x2 {
            for y in shape.y1..shape.y2 {
                colors.insert(*self.get_pixel(x as u32, y as u32));
            }
        }
        colors.len() == 1
    }

    fn qtree(&mut self, shape: Shape, block_id: Vec<usize>) {
        println!("In {} (aka block {})", shape, block_id_to_string(&block_id));
        if shape.height() < 20 || shape.width() < 20 || self.is_uniform_color(shape) {
            let color = self.average_color(shape);
            println!("Coloring {} (aka block {}) to {}", shape, block_id_to_string(&block_id), color);
            self.moves.push(Move::Color {
                block_id,
                color
            });
            return
        }
        let (x_mid, y_mid) = ((shape.x1 + shape.x2) / 2, (shape.y1 + shape.y2) / 2);
        let subblocks = [
            (0, Shape {x1: shape.x1, y1: shape.y1, x2: x_mid, y2: y_mid}),
            (1, Shape {x1: x_mid, y1: shape.y1, x2: shape.x2, y2: y_mid}),
            (2, Shape {x1: x_mid, y1: y_mid, x2: shape.x2, y2: shape.y2}),
            (3, Shape {x1: shape.x1, y1: y_mid, x2: x_mid, y2: shape.y2}),
        ];
        self.moves.push(PCut {
            block_id: block_id.clone(),
            x: x_mid,
            y: y_mid,
        });
        for (id, subshape) in subblocks {
            let mut new_block_id = block_id.clone();
            new_block_id.push(id);
            self.qtree(subshape, new_block_id)
        }
    }

}

fn qtree() {
    let problem_id = 2;
    let path = project_path(format!("data/problems/{}.png", problem_id));
    let img = read_img(path);
    let mut painter_state = PainterState::new(img.width() as i32, img.height() as i32);
    let mut state = State::new(img);
    state.solve();
    let mut out_file = File::create(project_path(format!("outputs/qtree{}.txt", problem_id))).unwrap();
    for mv in state.moves {
        painter_state.apply_move(&mv);
        println!("{}", mv);
        writeln!(out_file, "{}", mv).unwrap();
    }
    let res_img = painter_state.render();
    let out_path = project_path(format!("outputs/qtree{}.png", problem_id));
    res_img.save(out_path).unwrap();
}

