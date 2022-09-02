use std::fs::File;
use std::path::Path;
use image::{imageops, Rgba, RgbaImage};
use crate::image::Image;
use crate::basic::{BlockId, Color, image_distance, Move, PainterState, Shape};
use crate::basic::Move::PCut;
use crate::util::project_path;
use std::io::Write;

crate::entry_point!("qtree", qtree);

fn read_img(path: &Path) -> image::RgbaImage {
    let img = image::open(path).unwrap();
    img.to_rgba8()
}

fn crop(img: &RgbaImage, shape: Shape) -> RgbaImage {
    let c = imageops::crop_imm(
        img,
        shape.x1 as u32,
        (img.height() - shape.y2 as u32) as u32,
        (shape.x2 - shape.x1) as u32,
        (shape.y2 - shape.y1) as u32);
    c.to_image()
}

struct State {
    img: RgbaImage,
    painter_state: PainterState,
}

impl State {
    fn new(img: RgbaImage) -> State {
        let (width, height) = (img.width(), img.height());
        State {
            img,
            painter_state: PainterState::new(width as i32, height as i32),
        }
    }

    fn get_pixel(&self, x: u32, y: u32) -> &Rgba<u8> {
        self.img.get_pixel(x, self.img.height() - y - 1)
    }

    fn potential_gain(&self, painter_state: &PainterState, shape: Shape) -> i32 {
        let img = painter_state.render().to_raw_image();
        let cropped_actual = crop(&img, shape);
        let cropped_actual = Image::from_raw_image(&cropped_actual);
        let cropped_target = crop(&self.img, shape);
        let cropped_target = Image::from_raw_image(&cropped_target);
        // Sorry, these look like ugly and unnecessary conversions,
        // but I'm in a middle of a big refactoring (RgbaImage -> Image),
        // so doing what't easy to do.
        let res =
            image_distance(&cropped_target, &cropped_actual) as i32;
        println!("COMPARE {} {} {} {} = {}", cropped_target.width, cropped_target.height, cropped_actual.width, cropped_actual.height, res);
        res
    }

    fn solve(&mut self) -> (Vec<Move>, i32) {
        let shape = Shape {x1: 0, y1: 0, x2: self.img.width() as i32, y2: self.img.height() as i32 };
        let block = BlockId::root(0);
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
        Color([
            (r / pixels) as u8,
            (g / pixels) as u8,
            (b / pixels) as u8,
            (a / pixels) as u8,
        ])
    }

    fn qtree_stop_here_recolor_cost(&self, shape: Shape, block_id: BlockId) -> i32 {
        let mut state_copy = self.painter_state.clone();
        let extra_cost = state_copy.apply_move(&Move::Color {
            block_id,
            color: self.average_color(shape),
        });
        extra_cost + self.potential_gain(&state_copy, shape)
    }

    fn qtree_stop_here_no_recolor_cost(&self, shape: Shape) -> i32 {
        self.potential_gain(&self.painter_state, shape)
    }

    fn split_here_cost(&self, shape: Shape, block_id: BlockId) -> i32 {
        let mut state_copy = self.painter_state.clone();
        state_copy.apply_move(&PCut {
            block_id,
            x: (shape.x1 + shape.x2) / 2,
            y: (shape.y1 + shape.y2) / 2,
        })
    }

    fn qtree(&mut self, shape: Shape, block_id: BlockId) -> (Vec<Move>, i32) {
        let split_here = self.split_here_cost(shape, block_id.clone());
        let recolor_cost = self.qtree_stop_here_recolor_cost(shape, block_id.clone());
        let no_recolor_cost = self.qtree_stop_here_no_recolor_cost(shape);

        println!("In {} (aka block {}): repaint {}, no-op: {}, split: {}", shape, block_id, recolor_cost, no_recolor_cost, split_here);
        let (opt_moves, opt) = if no_recolor_cost > recolor_cost {
            (vec![Move::Color { block_id: block_id.clone(), color: self.average_color(shape) }], recolor_cost)
        } else {
            (vec![], no_recolor_cost)
        };

        if split_here > opt {
            println!("Splitting is too expensive at {} (block {}): {} > {}", shape, block_id, split_here, opt);
            return (opt_moves, opt)
        }

        let (x_mid, y_mid) = ((shape.x1 + shape.x2) / 2, (shape.y1 + shape.y2) / 2);
        let subblocks = [
            (0, Shape {x1: shape.x1, y1: shape.y1, x2: x_mid, y2: y_mid}),
            (1, Shape {x1: x_mid, y1: shape.y1, x2: shape.x2, y2: y_mid}),
            (2, Shape {x1: x_mid, y1: y_mid, x2: shape.x2, y2: shape.y2}),
            (3, Shape {x1: shape.x1, y1: y_mid, x2: x_mid, y2: shape.y2}),
        ];
        let mut moves = vec![PCut {
            block_id: block_id.clone(),
            x: x_mid,
            y: y_mid,
        }];
        self.painter_state.apply_move(&moves[0]);
        let mut cost = split_here;
        for (id, subshape) in subblocks {
            let new_block_id = block_id.child(id);
            let (mut new_moves, new_cost) = self.qtree(subshape, new_block_id);
            moves.append(&mut new_moves);
            cost += new_cost;
        }
        if opt < cost {
            (opt_moves, opt)
        } else {
            (moves, cost)
        }
    }

}

fn qtree() {
    let problem_id = 9;
    let path = project_path(format!("data/problems/{}.png", problem_id));

    let raw_img = read_img(&path);  // TODO: maybe get rid of image::RgbaImage altogether in favor of our Image
    let img = Image::load(&path);

    let mut painter_state = PainterState::new(img.width, img.height);
    let mut state = State::new(raw_img);
    let (moves, cost) = state.solve();
    let mut out_file = File::create(project_path(format!("outputs/qtree{}.txt", problem_id))).unwrap();
    for mv in moves {
        painter_state.apply_move(&mv);
        println!("{}", mv);
        writeln!(out_file, "{}", mv).unwrap();
    }
    let res_img = painter_state.render();
    let out_path = project_path(format!("outputs/qtree{}.png", problem_id));
    let img_dist = image_distance(&img, &res_img);
    println!("Distance cost: {}", img_dist);
    println!("Program cost:  {}", painter_state.cost);
    println!("Total cost:    {}", img_dist as u64 + painter_state.cost as u64);
    println!("Total cost2:   {}", cost);
    res_img.save(&out_path);
}
