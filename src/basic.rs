#![allow(unused_imports)]

use std::collections::HashMap;
use std::fmt::Write;

#[derive(Debug, Clone, Copy)]
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

#[derive(Debug, Clone)]
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
}

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
            shape: shape.clone(),
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
                let x = *x;
                let y = *y;
                let block = self.blocks.remove(block_id).unwrap();
                assert!(block.shape.x1 < x && x < block.shape.x2);
                assert!(block.shape.y1 < y && y < block.shape.y2);

                let new_block = block.sub_block(Shape {
                    x1: block.shape.x1,
                    y1: block.shape.y1,
                    x2: x,
                    y2: y,
                });
                let mut new_block_id = block_id.clone();
                new_block_id.push(0);
                self.blocks.insert(new_block_id, new_block);

                let new_block = block.sub_block(Shape {
                    x1: x,
                    y1: block.shape.y1,
                    x2: block.shape.x2,
                    y2: y,
                });
                let mut new_block_id = block_id.clone();
                new_block_id.push(1);
                self.blocks.insert(new_block_id, new_block);

                let new_block = block.sub_block(Shape {
                    x1: x,
                    y1: y,
                    x2: block.shape.x2,
                    y2: block.shape.y2,
                });
                let mut new_block_id = block_id.clone();
                new_block_id.push(2);
                self.blocks.insert(new_block_id, new_block);

                let new_block = block.sub_block(Shape {
                    x1: block.shape.x1,
                    y1: y,
                    x2: x,
                    y2: block.shape.y2,
                });
                let mut new_block_id = block_id.clone();
                new_block_id.push(3);
                self.blocks.insert(new_block_id, new_block);

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
                block.pieces = vec![(block.shape.clone(), *color)];
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
                let new_shape = if block1.shape.x2 == block2.shape.x1 {
                    assert_eq!(block1.shape.y1, block2.shape.y1);
                    assert_eq!(block1.shape.y2, block2.shape.y2);
                    Shape {
                        x1: block1.shape.x1,
                        y1: block1.shape.y1,
                        x2: block2.shape.x2,
                        y2: block2.shape.y2,
                    }
                } else {
                    panic!("merging blocks that are not adjacent {:?} {:?}", block1.shape, block2.shape);
                };
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

crate::entry_point!("render_moves_example", render_moves_example);
fn render_moves_example() {
    let moves = vec![
        PCut {
            block_id: vec![0],
            x: 200,
            y: 200
        },
        Move::Color {
            block_id: vec![0, 0],
            color: Color { r: 255, g: 0, b: 0, a: 255 },
        },
        Move::Color {
            block_id: vec![0, 2],
            color: Color { r: 0, g: 255, b: 0, a: 255 },
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
    let path = "outputs/render_moves_example.png";
    img.save(crate::util::project_path(path)).unwrap();
    eprintln!();
    eprintln!("saved to {}", path);
}
