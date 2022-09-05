#![allow(dead_code)]

use std::collections::HashMap;

use crate::basic::*;
use crate::image::Image;

#[derive(Clone, Copy)]
pub enum Transformation {
    TransposeXY,
    FlipY(i32)
}

fn transform_dimensions(width: i32, height: i32, t: &Transformation) -> (i32, i32) {
    match t {
        Transformation::TransposeXY => (height, width),
        Transformation::FlipY(_) => (width, height),
    }
}

impl Image {
    pub fn transform(&self, t: &Transformation) -> Image {
        let (w, h) = transform_dimensions(self.width, self.height, t);
        let mut res = Image::new(w, h, Color::default());
        for y in 0..self.height {
            for x in 0..self.width {
                let (x1, y1) = match t {
                    Transformation::TransposeXY => (y, x),
                    Transformation::FlipY(h) => {
                        assert_eq!(h, &self.height);
                        (x, h - 1 - y)
                    }
                };
                res.set_pixel(x1, y1, self.get_pixel(x, y));
            }
        }
        res
    }
}

impl Shape {
    pub fn transform(&self, t: &Transformation) -> Shape {
        match t {
            Transformation::TransposeXY =>
                Shape {x1: self.y1, y1: self.x1, x2: self.y2, y2: self.x2},
            Transformation::FlipY(h) =>
                Shape {x1: self.x1, y1: h - self.y2, x2: self.x2, y2: h - self.y1},
        }
    }
}

impl Pic {
    pub fn transform(&self, t: &Transformation) -> Pic {
        match self {
            Pic::Unicolor(_color) => self.clone(),
            Pic::Bitmap(shape) => Pic::Bitmap(shape.transform(t)),
        }
    }
}


impl Block {
    pub fn transform(&self, t: &Transformation) -> Block {
        let mut new_pieces = vec![];
        for (shape, pic) in &self.pieces {
            new_pieces.push((shape.transform(t), pic.transform(t)));
        }
        Block {shape: self.shape.transform(t), pieces: new_pieces}
    }
}

impl Orientation {
    pub fn transform(&self, t: &Transformation) -> Orientation {
        match t {
            Transformation::TransposeXY => {
                match self {
                    Orientation::Vertical => Orientation::Horizontal,
                    Orientation::Horizontal => Orientation::Vertical,
                }
            },
            Transformation::FlipY(_) => *self
        }
    }
}

fn transform_line_number(line_number: i32, orientation: &Orientation, t: &Transformation) -> i32 {
    match t {
        Transformation::TransposeXY => line_number,
        Transformation::FlipY(h) => {
            match orientation {
                Orientation::Vertical => line_number,
                Orientation::Horizontal => h - 1 - line_number,
            }
        }
    }
}

fn transform_pcut_coords(x: i32, y: i32, t: &Transformation) -> (i32, i32) {
    match t {
        Transformation::TransposeXY => (y, x),
        Transformation::FlipY(h) => (x, h - y),
    }
}


//fn dbg_print_map(block_id_map: &HashMap<BlockId, BlockId>) {
//    for (k, v) in block_id_map {
//        println!("{} -> {}", k, v);
//    }
//}

fn transform_move(m: &Move, t: &Transformation, block_id_map: &mut HashMap<BlockId, BlockId>) -> Move {
    //println!();
    //dbg_print_map(&block_id_map);
    match &m {
        Move::PCut {
            block_id,
            x,
            y,
        } => {
            let (x1, y1) = transform_pcut_coords(*x, *y, t);
            let new_block_id = block_id_map[block_id].clone();
            match t {
                Transformation::TransposeXY => {
                    block_id_map.insert(block_id.child(0), new_block_id.child(0));
                    block_id_map.insert(block_id.child(2), new_block_id.child(2));
                    block_id_map.insert(block_id.child(1), new_block_id.child(3));
                    block_id_map.insert(block_id.child(3), new_block_id.child(1));
                }
                Transformation::FlipY(_) => {
                    block_id_map.insert(block_id.child(0), new_block_id.child(3));
                    block_id_map.insert(block_id.child(3), new_block_id.child(0));
                    block_id_map.insert(block_id.child(1), new_block_id.child(2));
                    block_id_map.insert(block_id.child(2), new_block_id.child(1));
                }
            }
            Move::PCut {
                block_id: new_block_id,
                x: x1,
                y: y1,
            }
        }
        Move::LCut {
            block_id,
            orientation,
            line_number,
        } => {
            let new_block_id = block_id_map[block_id].clone();
            if orientation == &Orientation::Horizontal && matches!(t, Transformation::FlipY(_)) {
                block_id_map.insert(block_id.child(0), new_block_id.child(1));
                block_id_map.insert(block_id.child(1), new_block_id.child(0));
            } else {
                block_id_map.insert(block_id.child(0), new_block_id.child(0));
                block_id_map.insert(block_id.child(1), new_block_id.child(1));
            }
            Move::LCut {
                block_id: new_block_id,
                orientation: orientation.transform(t),
                line_number: transform_line_number(*line_number, orientation, t),
            }
        },
        Move::ColorMove {
            block_id,
            color,
        } => Move::ColorMove {
            block_id: block_id_map[block_id].clone(),
            color: *color,
        },
        Move::Swap {
            block_id1,
            block_id2,
        } => Move::Swap {
            block_id1: block_id_map[block_id1].clone(),
            block_id2: block_id_map[block_id2].clone(),
        },
        Move::Merge {
            block_id1,
            block_id2,
        } => Move::Merge {
            block_id1: block_id_map[block_id1].clone(),
            block_id2: block_id_map[block_id2].clone(),
        },
    }
}


pub fn transform_solution(moves: &Vec<Move>, t: &Transformation, start_blocks: &Vec<(BlockId, Block)>) -> Vec<Move> {
    let mut res = vec![];
    let mut block_id_map = HashMap::new();
    if start_blocks.is_empty() {
        block_id_map.insert(BlockId::root(0), BlockId::root(0));
    } else {
        for (block_id, _) in start_blocks {
            block_id_map.insert(block_id.clone(), block_id.clone());
        }
    }
    for m in moves {
        res.push(transform_move(m, t, &mut block_id_map));
    }
    res
}

fn assert_solutions_equal(moves1: &Vec<Move>, moves2: &Vec<Move>) {
    assert_eq!(moves1.len(), moves2.len());
    for (m1, m2) in moves1.iter().zip(moves2.iter()) {
        assert_eq!(m1, m2);
    }
}

//fn print_moves(moves: &Vec<Move>) {
//    for m in moves {
//        println!("{}", m);
//    }
//}

pub fn inverse_transform_sequence(sequence: &Vec<Transformation>) -> Vec<Transformation> {
    let mut res = vec![];
    for t in sequence {
        match t {
            Transformation::TransposeXY => res.push(Transformation::TransposeXY),
            Transformation::FlipY(h) => res.push(Transformation::FlipY(*h)),
        }
    }
    res.reverse();
    res
}

pub fn every_transform_sequence(h: i32) -> Vec<Vec<Transformation>> {
    let t = Transformation::TransposeXY;
    let f = Transformation::FlipY(h);
    vec![
        vec![],
        vec![t],
        vec![f],
        vec![t, f],
        vec![f, t],
        vec![f, t, f],
        vec![t, f, t],
        vec![f, t, f, t]
        ]
}

impl Problem {
    pub fn apply_transform_sequence(&self, sequence: &Vec<Transformation>) -> Problem {
        let mut res = self.clone();
        for t in sequence {
            res = res.transform(t);
        }
        res
    }
}

// Usage:
// for seq in every_transform_sequence(problem.height()) {
//     let transformed_problem = problem.apply_transform_sequence(&seq);
//     transformed_moves = solve(problem);
//     ...
//  }
// best_moves = apply_transform_sequence(best_transformed_moves, inverse_transform_sequence(best_seq), &problem.start_blocks);

#[test]
fn test_transform_solution() {
    let mut moves = vec![];

    moves.push(Move::PCut { block_id: BlockId::root(0), x: 120, y: 170 });
    moves.push(Move::LCut { block_id: BlockId::root(0).child(0), orientation: Orientation::Vertical, line_number: 30 });
    moves.push(Move::LCut { block_id: BlockId::root(0).child(1), orientation: Orientation::Horizontal, line_number: 40});
    moves.push(Move::ColorMove { block_id: BlockId::root(0).child(0).child(0), color: Color{0: [0,0,0,0]}});
    moves.push(Move::ColorMove { block_id: BlockId::root(0).child(0).child(1), color: Color{0: [0,0,0,1]}});
    moves.push(Move::ColorMove { block_id: BlockId::root(0).child(1).child(0), color: Color{0: [0,0,1,0]}});
    moves.push(Move::ColorMove { block_id: BlockId::root(0).child(1).child(1), color: Color{0: [0,0,1,1]}});
    moves.push(Move::ColorMove { block_id: BlockId::root(0).child(2), color: Color{0: [0,0,2,0]}});
    moves.push(Move::ColorMove { block_id: BlockId::root(0).child(3), color: Color{0: [0,0,3,0]}});

    //print_moves(&moves);

    let mut transformed_moves = moves.clone();
    for _ in 0..2 {
        transformed_moves = transform_solution(&transformed_moves, &Transformation::TransposeXY, &vec![]);
    }
    assert_solutions_equal(&moves, &transformed_moves);

    let mut transformed_moves = moves.clone();
    for _ in 0..2 {
        transformed_moves = transform_solution(&transformed_moves, &Transformation::FlipY(400), &vec![]);
    }
    assert_solutions_equal(&moves, &transformed_moves);

    let mut transformed_moves = moves.clone();
    transformed_moves = transform_solution(&transformed_moves, &Transformation::FlipY(400), &vec![]);
    transformed_moves = transform_solution(&transformed_moves, &Transformation::TransposeXY, &vec![]);
    transformed_moves = transform_solution(&transformed_moves, &Transformation::FlipY(400), &vec![]);
    transformed_moves = transform_solution(&transformed_moves, &Transformation::TransposeXY, &vec![]);
    transformed_moves = transform_solution(&transformed_moves, &Transformation::FlipY(400), &vec![]);
    transformed_moves = transform_solution(&transformed_moves, &Transformation::TransposeXY, &vec![]);
    transformed_moves = transform_solution(&transformed_moves, &Transformation::FlipY(400), &vec![]);
    transformed_moves = transform_solution(&transformed_moves, &Transformation::TransposeXY, &vec![]);
    assert_solutions_equal(&moves, &transformed_moves);

}

fn check_transform_e2e(problem_id: i32, sol: &str, tr: &Transformation) {
    let moves = Move::parse_many(sol);

    let problem = Problem::load(problem_id);
    let mut painter = PainterState::new(&problem);
    for m in &moves {
        painter.apply_move(m);
    }
    let img = painter.render();

    let transformed_problem = problem.transform(tr);
    let transformed_moves = transform_solution(&moves, tr, &problem.start_blocks);
    let mut transformed_painter = PainterState::new(&transformed_problem);
    for m in &transformed_moves {
        transformed_painter.apply_move(m);
    }
    let transformed_img = transformed_painter.render();

    assert_eq!(painter.cost, transformed_painter.cost);
    assert_eq!(img, transformed_img.transform(tr));
}

#[test]
fn test_transform_e2e() {
    for tr in &[
        Transformation::TransposeXY,
        Transformation::FlipY(400),
    ] {
        check_transform_e2e(1, "", tr);
        check_transform_e2e(30, "", tr);  // with initial bricks
        check_transform_e2e(36, "", tr);  // with initial painting

        check_transform_e2e(1, "color [0] [123, 34, 55, 255]", tr);

        check_transform_e2e(30, "merge [0] [1]", tr);

        check_transform_e2e(30, "swap [0] [1]", tr);

        check_transform_e2e(36, "
            cut [0] [x] [150]
            cut [0.1] [x] [190]
            merge [0.0] [0.1.0]
        ", tr);
        check_transform_e2e(36, "
            cut [0] [y] [150]
            cut [0.1] [y] [190]
            merge [0.0] [0.1.0]
        ", tr);

        for i in 0..4 {
            check_transform_e2e(37, &format!("
                cut [0] [120, 130]
                color [0.{i}] [123, 34, 55, 255]
            "), tr);
        }
    }
}

#[test]
fn transform_regr() {
    let moves2 = Move::parse_many("
        color [0] [128, 150, 152, 255]
        cut [0] [x] [373]
        color [0.0] [60, 83, 157, 255]
        cut [0.0] [x] [316]
        color [0.0.0] [48, 57, 58, 255]
        cut [0.0.0] [x] [81]
        color [0.0.0.1] [44, 59, 94, 255]
        cut [0.0.0.1] [x] [98]
        color [0.0.0.1.1] [104, 135, 164, 255]
        merge [0.0.0.1.1] [0.0.0.1.0]
        merge [1] [0.0.0.0]
        merge [2] [0.0.1]
        merge [3] [0.1]
        cut [4] [y] [54]
        color [4.1] [46, 62, 128, 255]
        cut [4.1] [x] [357]
        color [4.1.0] [61, 79, 128, 255]
        cut [4.1.0] [x] [353]
        color [4.1.0.0] [71, 90, 128, 255]
        cut [4.1.0.0] [x] [300]
        color [4.1.0.0.0] [82, 102, 121, 255]
        cut [4.1.0.0.0] [x] [238]
        color [4.1.0.0.0.0] [32, 38, 35, 255]
        cut [4.1.0.0.0.0] [x] [131]
        color [4.1.0.0.0.0.0] [29, 34, 31, 255]
        merge [4.1.0.0.0.0.0] [4.1.0.0.0.0.1]
        merge [5] [4.1.0.0.0.1]
        merge [6] [4.1.0.0.1]
        merge [7] [4.1.0.1]
        merge [8] [4.1.1]
        cut [9] [y] [103]
        color [9.1] [27, 34, 29, 255]
        cut [9.1] [x] [29]
        color [9.1.1] [29, 33, 30, 255]
        cut [9.1.1] [x] [50]
        color [9.1.1.1] [33, 34, 30, 255]
        cut [9.1.1.1] [x] [71]
        color [9.1.1.1.1] [28, 31, 28, 255]
        cut [9.1.1.1.1] [x] [85]
        color [9.1.1.1.1.1] [31, 35, 32, 255]
        cut [9.1.1.1.1.1] [x] [135]
        color [9.1.1.1.1.1.1] [72, 95, 118, 255]
        cut [9.1.1.1.1.1.1] [x] [163]
        color [9.1.1.1.1.1.1.1] [95, 125, 154, 255]
        cut [9.1.1.1.1.1.1.1] [x] [169]
        color [9.1.1.1.1.1.1.1.1] [48, 65, 135, 255]
        cut [9.1.1.1.1.1.1.1.1] [x] [347]
        color [9.1.1.1.1.1.1.1.1.0] [81, 109, 159, 255]
        cut [9.1.1.1.1.1.1.1.1.0] [x] [297]
        color [9.1.1.1.1.1.1.1.1.0.0] [92, 125, 160, 255]
        merge [9.1.1.1.1.1.1.1.1.0.0] [9.1.1.1.1.1.1.1.1.0.1]
        merge [10] [9.1.1.1.1.1.1.1.1.1]
        merge [11] [9.1.1.1.1.1.1.1.0]
        merge [12] [9.1.1.1.1.1.1.0]
        merge [13] [9.1.1.1.1.1.0]
        merge [14] [9.1.1.1.1.0]
        merge [15] [9.1.1.1.0]
        merge [16] [9.1.1.0]
        merge [17] [9.1.0]
        cut [18] [y] [142]
        color [18.1] [79, 101, 148, 255]
        cut [18.1] [x] [377]
        color [18.1.0] [63, 83, 144, 255]
        cut [18.1.0] [x] [350]
        color [18.1.0.0] [69, 95, 150, 255]
        cut [18.1.0.0] [x] [340]
        color [18.1.0.0.0] [89, 118, 163, 255]
        cut [18.1.0.0.0] [x] [331]
        color [18.1.0.0.0.0] [40, 47, 53, 255]
        cut [18.1.0.0.0.0] [x] [73]
        color [18.1.0.0.0.0.1] [60, 82, 118, 255]
        cut [18.1.0.0.0.0.1] [x] [145]
        color [18.1.0.0.0.0.1.1] [136, 161, 163, 255]
        cut [18.1.0.0.0.0.1.1] [x] [164]
        color [18.1.0.0.0.0.1.1.1] [110, 140, 165, 255]
        cut [18.1.0.0.0.0.1.1.1] [x] [197]
        color [18.1.0.0.0.0.1.1.1.1] [97, 128, 168, 255]
    ");

    let problem = Problem::load(17);
    let problem2 = problem.transform(&Transformation::TransposeXY);

    let mut painter2 = PainterState::new(&problem2);
    for m in &moves2 {
        painter2.apply_move(m);
    }
    // so far all ok

    // TODO: but this crashes
    // let moves = transform_solution(&moves2, &Transformation::TransposeXY, &problem2.start_blocks);
}
