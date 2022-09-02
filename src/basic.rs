#![allow(unused_imports)]

use std::fmt::Write;

pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

pub enum Orientation {
    Horizontal,
    Vertical,
}
use Orientation::*;

pub enum Move {
    PCut {
        block_id: Vec<usize>,
        x: i32,
        y: i32,
    },
    LCut {
        block_id: Vec<usize>,
        orientation: Orientation,
        line_number: i32,
    },
    Color {
        block_id: Vec<usize>,
        color: Color,
    },
    Swap {
        block_id1: Vec<usize>,
        block_id2: Vec<usize>,
    },
    Merge {
        block_id1: Vec<usize>,
        block_id2: Vec<usize>,
    },
}

use Move::*;

pub fn block_id_to_string(parts: &[usize]) -> String {
    parts.iter().map(|p| p.to_string()).collect::<Vec<String>>().join(",")
}

impl std::fmt::Display for Move {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PCut {
                block_id,
                x,
                y,
            } => write!(f, "cut [{}] [{}, {}]", block_id_to_string(block_id), x, y),
            LCut {
                block_id,
                orientation,
                line_number,
            } => {
                let o = match orientation {
                    Orientation::Horizontal => "y",
                    Orientation::Vertical => "x",
                };
                write!(f, "cut [{}] [{}] [{}]", block_id_to_string(block_id), o, line_number)
            },
            Move::Color {
                block_id,
                color,
            } => write!(f, "color [{}] [{}, {}, {}, {}]", block_id_to_string(block_id), color.r, color.g, color.b, color.a),
            Swap {
                block_id1,
                block_id2,
            } => write!(f, "swap [{}] [{}]", block_id_to_string(block_id1), block_id_to_string(block_id2)),
            Merge {
                block_id1,
                block_id2,
            } => write!(f, "merge [{}] [{}]", block_id_to_string(block_id1), block_id_to_string(block_id2)),
        }
    }
}

#[cfg(test)]
#[test]
fn test_move_to_string() {
    let moves = vec![
        Move::Color { block_id: vec![0], color: Color { r: 146, g: 149, b: 120, a: 223 } },
        Move::LCut { block_id: vec![0], orientation: Horizontal, line_number: 160 },
    ];

    let mut res = String::new();
    for m in moves {
        writeln!(res, "{}", m).unwrap();
    }

    assert_eq!(res, "\
color [0] [146, 149, 120, 223]
cut [0] [y] [160]
");
}
