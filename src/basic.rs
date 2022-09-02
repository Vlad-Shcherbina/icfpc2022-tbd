#![allow(unused_imports)]

use std::collections::HashMap;
use std::fmt::Write;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

#[derive(Debug, Clone, Copy)]
pub enum Orientation {
    Horizontal,
    Vertical,
}
use Orientation::*;

#[derive(Debug, Clone)]
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
    parts.iter().map(|p| p.to_string()).collect::<Vec<String>>().join(".")
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
        Move::Color { block_id: vec![0, 1], color: Color { r: 1, g: 2, b: 3, a: 4 } },
    ];

    let mut res = String::new();
    for m in moves {
        writeln!(res, "{}", m).unwrap();
    }

    assert_eq!(res, "\
color [0] [146, 149, 120, 223]
cut [0] [y] [160]
color [0.1] [1, 2, 3, 4]
");
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Shape {
    pub x1: i32,
    pub y1: i32,
    pub x2: i32,
    pub y2: i32,
}

impl Shape {
    pub fn width(&self) -> i32 {
        self.x2 - self.x1
    }
    pub fn height(&self) -> i32 {
        self.y2 - self.y1
    }
    pub fn size(&self) -> i32 {
        self.width() * self.height()
    }

    fn p_cut_subshapes(&self, x: i32, y: i32) -> [Shape; 4] {
        assert!(self.x1 < x && x < self.x2);
        assert!(self.y1 < y && y < self.y2);
        [
            Shape {
                x1: self.x1,
                y1: self.y1,
                x2: x,
                y2: y,
            },
            Shape {
                x1: x,
                y1: self.y1,
                x2: self.x2,
                y2: y,
            },
            Shape {
                x1: x,
                y1: y,
                x2: self.x2,
                y2: self.y2,
            },
            Shape {
                x1: self.x1,
                y1: y,
                x2: x,
                y2: self.y2,
            },
        ]
    }
}

#[derive(PartialEq)]
struct Block {
    shape: Shape,
    pieces: Vec<(Shape, Color)>,
}

impl Block {
    fn sub_block(&self, shape: Shape) -> Block {
        assert!(self.shape.x1 <= shape.x1);
        assert!(shape.x2 <= self.shape.x2);
        assert!(self.shape.y1 <= shape.y1);
        assert!(shape.y2 <= self.shape.y2);
        let mut pieces = vec![];
        for p in &self.pieces {
            if p.0.x2 <= shape.x1 || p.0.x1 >= shape.x2 ||
               p.0.y2 <= shape.y1 || p.0.y1 >= shape.y2 {
                continue;
            }
            pieces.push((
                Shape {
                    x1: p.0.x1.max(shape.x1),
                    y1: p.0.y1.max(shape.y1),
                    x2: p.0.x2.min(shape.x2),
                    y2: p.0.y2.min(shape.y2),
                },
                p.1,
            ));
        }
        Block {
            shape,
            pieces,
        }
    }
}

#[cfg(test)]
#[test]
fn test_sub_block() {
    let shape = Shape {x1: 10, y1: 110, x2: 20, y2: 120};
    let shape2 = Shape {x1: 11, y1: 111, x2: 13, y2: 113};
    let c = Color {r: 1, g: 2, b: 3, a: 4};
    let block = Block {
        shape: shape,
        pieces: vec![(shape.clone(), c)]
    };
    let subblock = block.sub_block(shape2.clone());
    assert!(subblock == Block {shape: shape2.clone(), pieces: vec![(shape2.clone(), c.clone())]})
}


pub struct PainterState {
    width: i32,
    height: i32,
    next_id: usize,
    blocks: HashMap<Vec<usize>, Block>,
    cost: i32,
}

impl PainterState {
    pub fn new(width: i32, height: i32) -> Self {
        let mut blocks = HashMap::new();
        let shape = Shape { x1: 0, y1: 0, x2: width, y2: height };
        blocks.insert(vec![0], Block {
            shape,
            pieces: vec![(shape, Color { r: 0, g: 0, b: 0, a: 0 })],
        });
        PainterState {
            width,
            height,
            next_id: 1,
            blocks,
            cost: 0,
        }
    }

    pub fn apply_move(&mut self, m: &Move) {
        let base_cost;
        let block_size;
        match m {
            PCut { block_id, x, y } => {
                let block = self.blocks.remove(block_id).unwrap();
                for (i, ss) in block.shape.p_cut_subshapes(*x, *y).into_iter().enumerate() {
                    let new_block = block.sub_block(ss);
                    let mut new_block_id = block_id.clone();
                    new_block_id.push(i);
                    self.blocks.insert(new_block_id, new_block);
                }
                base_cost = 10;
                block_size = block.shape.size();
            }
            LCut { block_id, orientation, line_number } => {
                let line_number = *line_number;
                let block = self.blocks.remove(block_id).unwrap();
                match orientation {
                    Horizontal => {
                        assert!(block.shape.y1 < line_number && line_number < block.shape.y2);

                        let new_block = block.sub_block(Shape {
                            x1: block.shape.x1,
                            y1: block.shape.y1,
                            x2: block.shape.x2,
                            y2: line_number,
                        });
                        let mut new_block_id = block_id.clone();
                        new_block_id.push(0);
                        self.blocks.insert(new_block_id, new_block);

                        let new_block = block.sub_block(Shape {
                            x1: block.shape.x1,
                            y1: line_number,
                            x2: block.shape.x2,
                            y2: block.shape.y2,
                        });
                        let mut new_block_id = block_id.clone();
                        new_block_id.push(1);
                        self.blocks.insert(new_block_id, new_block);
                    }
                    Vertical => {
                        assert!(block.shape.x1 < line_number && line_number < block.shape.x2);

                        let new_block = block.sub_block(Shape {
                            x1: block.shape.x1,
                            y1: block.shape.y1,
                            x2: line_number,
                            y2: block.shape.y2,
                        });
                        let mut new_block_id = block_id.clone();
                        new_block_id.push(0);
                        self.blocks.insert(new_block_id, new_block);

                        let new_block = block.sub_block(Shape {
                            x1: line_number,
                            y1: block.shape.y1,
                            x2: block.shape.x2,
                            y2: block.shape.y2,
                        });
                        let mut new_block_id = block_id.clone();
                        new_block_id.push(1);
                        self.blocks.insert(new_block_id, new_block);
                    }
                }
                base_cost = 7;
                block_size = block.shape.size();
            }
            Move::Color { block_id, color } => {
                let block = self.blocks.get_mut(block_id).unwrap();
                block.pieces = vec![(block.shape, *color)];
                base_cost = 5;
                block_size = block.shape.size();
            }
            Swap { block_id1, block_id2 } => {
                let mut block1 = self.blocks.remove(block_id1).unwrap();
                let mut block2 = self.blocks.remove(block_id2).unwrap();
                assert_eq!(block1.shape.width(), block2.shape.width());
                assert_eq!(block1.shape.height(), block2.shape.height());

                let dx = block2.shape.x1 - block1.shape.x1;
                let dy = block2.shape.y1 - block1.shape.y1;

                for p in &mut block1.pieces {
                    p.0.x1 += dx;
                    p.0.x2 += dx;
                    p.0.y1 += dy;
                    p.0.y2 += dy;
                }
                for p in &mut block2.pieces {
                    p.0.x1 -= dx;
                    p.0.x2 -= dx;
                    p.0.y1 -= dy;
                    p.0.y2 -= dy;
                }
                std::mem::swap(&mut block1.shape, &mut block2.shape);

                base_cost = 3;
                block_size = block1.shape.size();

                self.blocks.insert(block_id1.clone(), block1);
                self.blocks.insert(block_id2.clone(), block2);
            }
            Merge { block_id1, block_id2 } => {
                let block1 = self.blocks.remove(block_id1).unwrap();
                let block2 = self.blocks.remove(block_id2).unwrap();
                let new_shape = merge_shapes(block1.shape, block2.shape);
                let mut new_block = Block {
                    shape: new_shape,
                    pieces: block1.pieces,
                };
                new_block.pieces.extend(block2.pieces);

                base_cost = 1;
                block_size = new_block.shape.size();

                self.blocks.insert(vec![self.next_id], new_block);
                self.next_id += 1;
            }
        }
        self.cost += (base_cost * self.width * self.height + (block_size + 1)/2) / block_size;
    }

    pub fn render(&self) -> image::RgbaImage {
        let mut res = image::RgbaImage::new(self.width as u32, self.height as u32);
        for block in self.blocks.values() {
            for (shape, color) in &block.pieces {
                let c = image::Rgba([color.r, color.g, color.b, color.a]);
                for x in shape.x1..shape.x2 {
                    for y in shape.y1..shape.y2 {
                        res.put_pixel(x as u32, (self.height - 1 - y) as u32, c);
                    }
                }
            }
        }
        res
    }
}

fn merge_shapes(shape1: Shape, shape2: Shape) -> Shape {
    if shape1.x2 == shape2.x1 && shape1.y1 == shape2.y1 && shape1.y2 == shape2.y2 {
        // 1 to the left of 2
        Shape {
            x1: shape1.x1,
            y1: shape1.y1,
            x2: shape2.x2,
            y2: shape2.y2,
        }
    } else if shape2.x1 == shape1.x2 && shape1.y1 == shape2.y1 && shape1.y2 == shape2.y2 {
        // 2 to the left of 1
        Shape {
            x1: shape2.x1,
            y1: shape2.y1,
            x2: shape1.x2,
            y2: shape1.y2,
        }
    } else if shape1.y2 == shape2.y1 && shape1.x1 == shape2.x1 && shape1.x2 == shape2.x2 {
        // 2 on top of 1
        Shape {
            x1: shape1.x1,
            y1: shape1.y1,
            x2: shape2.x2,
            y2: shape2.y2,
        }
    } else if shape2.y2 == shape1.y1 && shape1.x1 == shape2.x1 && shape1.x2 == shape2.x2 {
        // 1 on top of 2
        Shape {
            x1: shape2.x1,
            y1: shape2.y1,
            x2: shape1.x2,
            y2: shape1.y2,
        }
    } else {
        panic!("merging blocks that are not adjacent {:?} {:?}", shape1, shape2);
    }
}

pub fn image_distance(img1: &image::RgbaImage, img2: &image::RgbaImage) -> f64 {
    assert_eq!(img1.width(), img2.width());
    assert_eq!(img1.height(), img2.height());
    let mut res = 0.0f64;
    for (x, y, pixel1) in img1.enumerate_pixels() {
        let pixel2 = img2.get_pixel(x, y);
        let mut d2 = 0;
        for i in 0..4 {
            d2 += (pixel1[i] as i32 - pixel2[i] as i32).pow(2);
        }
        res += (d2 as f64).sqrt();
    }
    res *= 0.05;
    res
}

crate::entry_point!("render_moves_example", render_moves_example);
fn render_moves_example() {
    let moves = vec![
        Move::Color {
            block_id: vec![0],
            color: Color { r: 0, g: 0, b: 255, a: 255 },
        },
        PCut {
            block_id: vec![0],
            x: 360,
            y: 40
        },
        Move::Color {
            block_id: vec![0, 3],
            color: Color { r: 128, g: 128, b: 128, a: 255 },
        },
    ];
    let mut painter = PainterState::new(400, 400);
    for m in &moves {
        eprintln!("{}", m);
        painter.apply_move(m);
    }
    eprintln!();
    eprintln!("cost: {}", painter.cost);

    let img = painter.render();

    let target_path = "data/problems/1.png";
    let target = image::open(crate::util::project_path(target_path)).unwrap().to_rgba8();
    let dist = image_distance(&img, &target);
    eprintln!("distance to {} is {}", target_path, dist);

    let path = "outputs/render_moves_example.png";
    img.save(crate::util::project_path(path)).unwrap();
    eprintln!();
    eprintln!("saved to {}", path);
}

#[cfg(test)]
#[test]
fn test_split_merge() {
    let mut painter = PainterState::new(100, 100);
    painter.apply_move(&LCut {
        block_id: vec![0],
        orientation: Orientation::Horizontal,
        line_number: 50,
    });
    painter.apply_move(&Merge { block_id1: vec![0, 0], block_id2: vec![0, 1] })
}


