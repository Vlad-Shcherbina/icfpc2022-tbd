#![allow(unused_imports)]
#![allow(dead_code)]

use crate::image::Image;
use crate::transform::Transformation;
use crate::util::project_path;
use std::cmp::{max, min};
use fxhash::FxHashMap as HashMap;
use std::fmt::{Formatter, Write};

#[derive(Clone, Copy, PartialEq, Eq, Default, Hash, PartialOrd, Ord)]
pub struct Color(pub [u8; 4]);

impl Color {
    pub fn dist(&self, other: &Color) -> f64 {
        let mut d = 0;
        for i in 0..4 {
            d += (self.0[i] as i32 - other.0[i] as i32).pow(2);
        }
        (d as f64).sqrt()
    }

    pub fn next(&self) -> Color {
        Color((u32::from_be_bytes(self.0) + 1).to_be_bytes())
    }
}

impl std::fmt::Debug for Color {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Color({:?})", self.0)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Orientation {
    Horizontal,
    Vertical,
}
use Orientation::*;

#[derive(Debug, Clone, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct BlockId(Vec<usize>);

impl BlockId {
    pub fn root(idx: usize) -> BlockId {
        BlockId(vec![idx])
    }
    pub fn child(&self, idx: usize) -> BlockId {
        let mut res = self.clone();
        res.0.push(idx);
        res
    }
    pub fn is_root(&self) -> bool {
        self.0.len() == 1
    }

    pub fn parse(s: &str) -> BlockId {
        BlockId(s.split('.').map(|p| p.parse().unwrap()).collect())
    }
}

impl std::fmt::Display for BlockId {
    fn fmt(&self, f: &mut Formatter) -> std::fmt::Result {
        for (i, part) in self.0.iter().enumerate() {
            if i > 0 {
                write!(f, ".")?;
            }
            write!(f, "{}", part)?;
        }
        Ok(())
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Move {
    PCut {
        block_id: BlockId,
        x: i32,
        y: i32,
    },
    LCut {
        block_id: BlockId,
        orientation: Orientation,
        line_number: i32,
    },
    ColorMove {
        block_id: BlockId,
        color: Color,
    },
    Swap {
        block_id1: BlockId,
        block_id2: BlockId,
    },
    Merge {
        block_id1: BlockId,
        block_id2: BlockId,
    },
}

use crate::basic::PainterStateAction::{
    AddBlock, ColorBlock, IncrementCost, IncrementNextId, RemoveBlock, SwapBlocks,
};
use Move::*;

impl std::fmt::Display for Move {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PCut { block_id, x, y } => write!(f, "cut [{}] [{}, {}]", block_id, x, y),
            LCut {
                block_id,
                orientation,
                line_number,
            } => {
                let o = match orientation {
                    Orientation::Horizontal => "y",
                    Orientation::Vertical => "x",
                };
                write!(f, "cut [{}] [{}] [{}]", block_id, o, line_number)
            }
            ColorMove { block_id, color } => write!(
                f,
                "color [{}] [{}, {}, {}, {}]",
                block_id, color.0[0], color.0[1], color.0[2], color.0[3]
            ), // TODO: use color as Display
            Swap {
                block_id1,
                block_id2,
            } => write!(f, "swap [{}] [{}]", block_id1, block_id2),
            Merge {
                block_id1,
                block_id2,
            } => write!(f, "merge [{}] [{}]", block_id1, block_id2),
        }
    }
}

impl Move {
    pub fn parse_many(s: &str) -> Vec<Move> {
        let mut moves = vec![];
        for line in s.split_terminator('\n') {
            let line = line.trim();
            if line.is_empty() {
                continue;
            }
            if line.starts_with('#') {
                continue;
            }
            moves.push(Move::parse(line));
        }
        moves
    }
    pub fn parse(s: &str) -> Move {
        let s = s.replace(' ', "");
        if let Some(s) = s.strip_prefix("color") {
            let (block_id, s) = strip_block_id(s);
            let s = s.strip_prefix('[').unwrap();
            let s = s.strip_suffix(']').unwrap();
            let mut it = s.split(',');
            let color = Color([
                it.next().unwrap().parse().unwrap(),
                it.next().unwrap().parse().unwrap(),
                it.next().unwrap().parse().unwrap(),
                it.next().unwrap().parse().unwrap(),
            ]);
            assert!(it.next().is_none());
            return Move::ColorMove { block_id, color };
        }
        if let Some(s) = s.strip_prefix("cut") {
            let (block_id, s) = strip_block_id(s);
            if let Some(s) = s.strip_prefix("[x]") {
                let line_number = s
                    .strip_prefix('[')
                    .unwrap()
                    .strip_suffix(']')
                    .unwrap()
                    .parse()
                    .unwrap();
                return Move::LCut {
                    block_id,
                    orientation: Vertical,
                    line_number,
                };
            } else if let Some(s) = s.strip_prefix("[y]") {
                let line_number = s
                    .strip_prefix('[')
                    .unwrap()
                    .strip_suffix(']')
                    .unwrap()
                    .parse()
                    .unwrap();
                return Move::LCut {
                    block_id,
                    orientation: Horizontal,
                    line_number,
                };
            } else {
                let s = s.strip_prefix('[').unwrap();
                let s = s.strip_suffix(']').unwrap();
                let mut it = s.split(',');
                let x = it.next().unwrap().parse().unwrap();
                let y = it.next().unwrap().parse().unwrap();
                assert!(it.next().is_none());
                return Move::PCut { block_id, x, y };
            }
        }
        if let Some(s) = s.strip_prefix("swap") {
            let (block_id1, s) = strip_block_id(s);
            let (block_id2, s) = strip_block_id(s);
            assert_eq!(s, "");
            return Move::Swap {
                block_id1,
                block_id2,
            };
        }
        if let Some(s) = s.strip_prefix("merge") {
            let (block_id1, s) = strip_block_id(s);
            let (block_id2, s) = strip_block_id(s);
            assert_eq!(s, "");
            return Move::Merge {
                block_id1,
                block_id2,
            };
        }
        panic!("unrecognized move {:?}", s);
    }
}

fn strip_block_id(s: &str) -> (BlockId, &str) {
    let s = s.strip_prefix('[').unwrap();
    let (block_id, s) = s.split_once(']').unwrap();
    (BlockId::parse(block_id), s)
}

fn roundtrip(s: &str) {
    let m = Move::parse(s);
    let s2 = format!("{}", m);
    assert_eq!(s, s2);
}

#[test]
fn test_string_to_move_and_back() {
    roundtrip("color [0.1] [146, 149, 120, 223]");
    roundtrip("cut [0.1] [x] [11]");
    roundtrip("cut [0.1] [y] [42]");
    roundtrip("cut [0.1] [42, 11]");
    roundtrip("swap [0.1] [7.2]");
    roundtrip("merge [0.1] [7.2]");
}

#[cfg(test)]
#[test]
fn test_move_to_string() {
    let moves = vec![
        Move::ColorMove {
            block_id: BlockId::root(0),
            color: Color([146, 149, 120, 223]),
        },
        Move::LCut {
            block_id: BlockId::root(0),
            orientation: Horizontal,
            line_number: 160,
        },
        Move::ColorMove {
            block_id: BlockId::root(0).child(1),
            color: Color([1, 2, 3, 4]),
        },
    ];

    let mut res = String::new();
    for m in moves {
        writeln!(res, "{}", m).unwrap();
    }

    assert_eq!(
        res,
        "\
color [0] [146, 149, 120, 223]
cut [0] [y] [160]
color [0.1] [1, 2, 3, 4]
"
    );
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct Shape {
    pub x1: i32,
    pub y1: i32,
    pub x2: i32,
    pub y2: i32,
}

impl std::fmt::Display for Shape {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "shape([{}, {}]x[{}, {}])",
            self.x1, self.x2, self.y1, self.y2
        )
    }
}

impl std::fmt::Display for Color {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "[{}, {}, {}, {}]",
            self.0[0], self.0[1], self.0[2], self.0[3]
        )
    }
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

    pub fn p_cut_subshapes(&self, x: i32, y: i32) -> [Shape; 4] {
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

    pub fn l_cut_subshapes(&self, orientation: Orientation, line_number: i32) -> [Shape; 2] {
        match orientation {
            Horizontal => {
                assert!(self.y1 < line_number && line_number < self.y2);
                [
                    Shape {
                        x1: self.x1,
                        y1: self.y1,
                        x2: self.x2,
                        y2: line_number,
                    },
                    Shape {
                        x1: self.x1,
                        y1: line_number,
                        x2: self.x2,
                        y2: self.y2,
                    },
                ]
            }
            Vertical => {
                assert!(self.x1 < line_number && line_number < self.x2);
                [
                    Shape {
                        x1: self.x1,
                        y1: self.y1,
                        x2: line_number,
                        y2: self.y2,
                    },
                    Shape {
                        x1: line_number,
                        y1: self.y1,
                        x2: self.x2,
                        y2: self.y2,
                    },
                ]
            }
        }
    }

    pub fn contains(&self, other: Shape) -> bool {
        self.x1 <= other.x1 && other.x2 <= self.x2 && self.y1 <= other.y1 && other.y2 <= self.y2
    }

    pub fn intersect(&self, other: &Shape) -> Option<Shape> {
        let x1 = max(self.x1, other.x1);
        let y1 = max(self.y1, other.y1);
        let x2 = min(self.x2, other.x2);
        let y2 = min(self.y2, other.y2);
        if x1 < x2 && y1 < y2 {
            Some(Shape { x1, y1, x2, y2 })
        } else {
            None
        }
    }

    pub(crate) fn from_image(img: &Image) -> Shape {
        Shape {
            x1: 0,
            y1: 0,
            x2: img.width,
            y2: img.height,
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Pic {
    Unicolor(Color),
    Bitmap(Shape),
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Block {
    pub shape: Shape,
    pub pieces: Vec<(Shape, Pic)>,
}

impl Block {
    fn sub_block(&self, shape: Shape) -> Block {
        assert!(self.shape.x1 <= shape.x1);
        assert!(shape.x2 <= self.shape.x2);
        assert!(self.shape.y1 <= shape.y1);
        assert!(shape.y2 <= self.shape.y2);
        let mut pieces = vec![];
        for p in &self.pieces {
            if p.0.x2 <= shape.x1 || p.0.x1 >= shape.x2 || p.0.y2 <= shape.y1 || p.0.y1 >= shape.y2
            {
                continue;
            }
            let new_shape = Shape {
                x1: p.0.x1.max(shape.x1),
                y1: p.0.y1.max(shape.y1),
                x2: p.0.x2.min(shape.x2),
                y2: p.0.y2.min(shape.y2),
            };
            let sub_pic = match &p.1 {
                Pic::Unicolor(c) => Pic::Unicolor(*c),
                Pic::Bitmap(s) => Pic::Bitmap(Shape {
                    x1: s.x1 + new_shape.x1 - p.0.x1,
                    y1: s.y1 + new_shape.y1 - p.0.y1,
                    x2: s.x2 + new_shape.x2 - p.0.x2,
                    y2: s.y2 + new_shape.y2 - p.0.y2,
                }),
            };
            pieces.push((new_shape, sub_pic));
        }
        Block { shape, pieces }
    }
}

#[cfg(test)]
#[test]
fn test_sub_block() {
    let shape = Shape {
        x1: 10,
        y1: 110,
        x2: 20,
        y2: 120,
    };
    let shape2 = Shape {
        x1: 11,
        y1: 111,
        x2: 13,
        y2: 113,
    };
    let c = Color([1, 2, 3, 4]);
    let block = Block {
        shape: shape,
        pieces: vec![(shape.clone(), Pic::Unicolor(c))],
    };
    let subblock = block.sub_block(shape2.clone());
    assert!(
        subblock
            == Block {
                shape: shape2.clone(),
                pieces: vec![(shape2.clone(), Pic::Unicolor(c))]
            }
    )
}

#[derive(Clone, Debug, PartialEq, Eq)]
enum PainterStateAction {
    RemoveBlock {
        block_id: BlockId,
        removed_value: Block,
    },
    AddBlock {
        block_id: BlockId,
    },
    IncrementNextId,
    IncrementCost {
        added_cost: i64,
    },
    ColorBlock {
        block_id: BlockId,
        old_block: Block,
    },
    SwapBlocks {
        block_id1: BlockId,
        block_id2: BlockId,
    },
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BaseCosts {
    pub lcut: i64,
    pub pcut: i64,
    pub merge: i64,
    pub swap: i64,
    pub color: i64,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PainterState<'a> {
    initial_img: Option<&'a Image>,
    base_costs: BaseCosts,
    width: i32,
    height: i32,
    next_id: usize,
    pub blocks: HashMap<BlockId, Block>,
    pub(crate) cost: i64,
    history: Vec<Vec<PainterStateAction>>,
    snapshot: Option<usize>, // target `history` size
    pub moves: Vec<Move>,
}

pub struct ApplyMoveResult {
    pub cost: i64,
    pub new_block_ids: Vec<BlockId>, // new block IDs for "cut" and "merge" moves
}

impl<'a> PainterState<'a> {
    pub fn new(p: &'a Problem) -> Self {
        for (id, _b) in &p.start_blocks {
            assert_eq!(id.0.len(), 1);
        }
        let next_id = p.start_blocks.iter().map(|(id, _)| id.0[0]).max().unwrap() + 1;
        PainterState {
            initial_img: p.initial_img.as_ref(),
            base_costs: p.base_costs.clone(),
            width: p.width,
            height: p.height,
            next_id,
            blocks: p.start_blocks.iter().cloned().collect(),
            cost: 0,
            history: vec![],
            snapshot: None,
            moves: vec![],
        }
    }

    pub fn rollback_move(&mut self) {
        assert!(!self.history.is_empty());
        let mut last_move_actions = self.history.pop().unwrap();
        while let Some(action) = last_move_actions.pop() {
            match action {
                PainterStateAction::RemoveBlock {
                    block_id,
                    removed_value,
                } => {
                    self.blocks.insert(block_id, removed_value);
                }
                PainterStateAction::AddBlock { block_id } => {
                    self.blocks.remove(&block_id).unwrap();
                }
                PainterStateAction::IncrementNextId => {
                    self.next_id -= 1;
                }
                PainterStateAction::IncrementCost { added_cost } => {
                    self.cost -= added_cost;
                }
                ColorBlock {
                    block_id,
                    old_block,
                } => {
                    self.blocks.insert(block_id, old_block);
                }
                SwapBlocks {
                    block_id1,
                    block_id2,
                } => {
                    let mut block1 = self.blocks.remove(&block_id1).unwrap();
                    let mut block2 = self.blocks.remove(&block_id2).unwrap();
                    swap_blocks(&mut block1, &mut block2);
                    self.blocks.insert(block_id1.clone(), block1);
                    self.blocks.insert(block_id2.clone(), block2);
                }
            }
        }
        self.moves.pop();
    }

    // Returns the cost of the applied move
    pub fn apply_move(&mut self, m: &Move) -> ApplyMoveResult {
        let mut new_block_ids = vec![];
        let base_cost;
        let block_size;
        let mut actions = vec![];
        match m {
            PCut { block_id, x, y } => {
                let block = self.blocks.remove(block_id).unwrap();
                actions.push(RemoveBlock {
                    block_id: block_id.clone(),
                    removed_value: block.clone(),
                });
                for (i, ss) in block.shape.p_cut_subshapes(*x, *y).into_iter().enumerate() {
                    let new_block = block.sub_block(ss);
                    let new_block_id = block_id.child(i);
                    actions.push(AddBlock {
                        block_id: new_block_id.clone(),
                    });
                    new_block_ids.push(new_block_id.clone());
                    self.blocks.insert(new_block_id, new_block);
                }
                base_cost = self.base_costs.pcut;
                block_size = block.shape.size();
            }
            LCut {
                block_id,
                orientation,
                line_number,
            } => {
                let block = self.blocks.remove(block_id).unwrap();
                actions.push(RemoveBlock {
                    block_id: block_id.clone(),
                    removed_value: block.clone(),
                });
                for (i, ss) in block
                    .shape
                    .l_cut_subshapes(*orientation, *line_number)
                    .into_iter()
                    .enumerate()
                {
                    let new_block = block.sub_block(ss);
                    let new_block_id = block_id.child(i);
                    actions.push(AddBlock {
                        block_id: new_block_id.clone(),
                    });
                    new_block_ids.push(new_block_id.clone());
                    self.blocks.insert(new_block_id, new_block);
                }
                base_cost = self.base_costs.lcut;
                block_size = block.shape.size();
            }
            Move::ColorMove { block_id, color } => {
                let block = self.blocks.get_mut(block_id).unwrap();
                actions.push(ColorBlock {
                    block_id: block_id.clone(),
                    old_block: block.clone(),
                });
                block.pieces = vec![(block.shape, Pic::Unicolor(*color))];
                base_cost = self.base_costs.color;
                block_size = block.shape.size();
            }
            Swap {
                block_id1,
                block_id2,
            } => {
                let mut block1 = self.blocks.remove(block_id1).unwrap();
                let mut block2 = self.blocks.remove(block_id2).unwrap();
                swap_blocks(&mut block1, &mut block2);
                base_cost = self.base_costs.swap;
                block_size = block1.shape.size();
                self.blocks.insert(block_id1.clone(), block1);
                self.blocks.insert(block_id2.clone(), block2);
                actions.push(SwapBlocks {
                    block_id1: block_id1.clone(),
                    block_id2: block_id2.clone(),
                })
            }
            Merge {
                block_id1,
                block_id2,
            } => {
                let block1 = self.blocks.remove(block_id1).unwrap();
                let block2 = self.blocks.remove(block_id2).unwrap();
                actions.push(RemoveBlock {
                    block_id: block_id1.clone(),
                    removed_value: block1.clone(),
                });
                actions.push(RemoveBlock {
                    block_id: block_id2.clone(),
                    removed_value: block2.clone(),
                });
                let mut new_block = Block {
                    shape: merge_shapes(block1.shape, block2.shape).unwrap_or_else(|| {
                        panic!(
                            "merging blocks that are not adjacent {:?} {:?} (in move {})",
                            block1.shape, block2.shape, m
                        )
                    }),
                    pieces: block1.pieces,
                };
                new_block.pieces.extend(block2.pieces);

                base_cost = self.base_costs.merge;
                block_size = block1.shape.size().max(block2.shape.size());

                let new_block_id = BlockId::root(self.next_id);
                new_block_ids.push(new_block_id.clone());
                actions.push(AddBlock {
                    block_id: new_block_id.clone(),
                });
                actions.push(IncrementNextId);
                self.blocks.insert(new_block_id, new_block);
                self.next_id += 1;
            }
        }
        let extra_cost =
            (base_cost * (self.width * self.height) as i64 + (block_size as i64 + 1) / 2) / block_size as i64;
        self.cost += extra_cost;
        actions.push(IncrementCost {
            added_cost: extra_cost,
        });
        self.history.push(actions);
        self.moves.push(m.clone());
        ApplyMoveResult {
            cost: extra_cost,
            new_block_ids,
        }
    }

    pub fn render(&self) -> Image {
        let _t = crate::stats_timer!("render").time_it();
        let mut res = Image::new(self.width, self.height, Color::default());
        for block in self.blocks.values() {
            for (shape, pic) in &block.pieces {
                match pic {
                    Pic::Unicolor(color) => {
                        for y in shape.y1..shape.y2 {
                            for x in shape.x1..shape.x2 {
                                res.set_pixel(x, y, *color);
                            }
                        }
                    }
                    Pic::Bitmap(s) => {
                        assert_eq!(s.width(), shape.width());
                        assert_eq!(s.height(), shape.height());
                        let img = self.initial_img.unwrap();
                        let dx = s.x1 - shape.x1;
                        let dy = s.y1 - shape.y1;
                        for y in shape.y1..shape.y2 {
                            for x in shape.x1..shape.x2 {
                                res.set_pixel(x, y, img.get_pixel(x + dx, y + dy));
                            }
                        }
                    }
                }
            }
        }
        res
    }

    pub fn snapshot(&mut self) {
        assert!(self.snapshot.is_none());
        self.snapshot = Some(self.history.len());
    }

    pub fn rollback(&mut self) {
        assert!(self.snapshot.is_some());
        let target_size = self.snapshot.unwrap();
        while self.history.len() != target_size {
            self.rollback_move()
        }
        self.snapshot = None
    }
}

pub fn merge_shapes(shape1: Shape, shape2: Shape) -> Option<Shape> {
    if shape1.intersect(&shape2).is_some() {
        None
    } else if shape1.x2 == shape2.x1 && shape1.y1 == shape2.y1 && shape1.y2 == shape2.y2 {
        // 1 to the left of 2
        Some(Shape {
            x1: shape1.x1,
            y1: shape1.y1,
            x2: shape2.x2,
            y2: shape2.y2,
        })
    } else if shape2.x2 == shape1.x1 && shape1.y1 == shape2.y1 && shape1.y2 == shape2.y2 {
        // 2 to the left of 1
        Some(Shape {
            x1: shape2.x1,
            y1: shape2.y1,
            x2: shape1.x2,
            y2: shape1.y2,
        })
    } else if shape1.y2 == shape2.y1 && shape1.x1 == shape2.x1 && shape1.x2 == shape2.x2 {
        // 2 on top of 1
        Some(Shape {
            x1: shape1.x1,
            y1: shape1.y1,
            x2: shape2.x2,
            y2: shape2.y2,
        })
    } else if shape2.y2 == shape1.y1 && shape1.x1 == shape2.x1 && shape1.x2 == shape2.x2 {
        // 1 on top of 2
        Some(Shape {
            x1: shape2.x1,
            y1: shape2.y1,
            x2: shape1.x2,
            y2: shape1.y2,
        })
    } else {
        None
    }
}

fn swap_blocks(block1: &mut Block, block2: &mut Block) {
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
}

pub fn image_slice_distance_to_color(img: &Image, shape: Shape, color: &Color) -> f64 {
    let mut res = 0.0f64;
    for y in shape.y1..shape.y2 {
        for x in shape.x1..shape.x2 {
            res += img.get_pixel(x, y).dist(color);
        }
    }
    res *= 0.005;
    res
}

pub fn image_slices_distance(img1: &Image, img2: &Image, shape1: Shape, shape2: Shape) -> f64 {
    assert!(shape1.width() == shape2.width());
    assert!(shape1.height() == shape2.height());
    let mut res = 0.0f64;
    for dy in 0..shape1.height() {
        for dx in 0..shape1.width() {
            res += img1
                .get_pixel(shape1.x1 + dx, shape1.y1 + dy)
                .dist(&img2.get_pixel(shape2.x1 + dx, shape2.y1 + dy));
        }
    }
    res *= 0.005;
    res
}

pub fn image_slice_distance(img1: &Image, img2: &Image, shape: Shape) -> f64 {
    image_slices_distance(img1, img2, shape, shape)
}

pub fn image_distance(img1: &Image, img2: &Image) -> f64 {
    assert_eq!(img1.width, img2.width);
    assert_eq!(img1.height, img2.height);
    image_slice_distance(
        img1,
        img2,
        Shape {
            x1: 0,
            y1: 0,
            x2: img1.width,
            y2: img1.height,
        },
    )
}

#[derive(serde::Deserialize, Debug)]
struct InitialCanvas {
    width: i32,
    height: i32,
    blocks: Vec<InitialBlock>,
}

#[derive(serde::Deserialize, Debug)]
struct InitialBlock {
    #[serde(rename = "blockId")]
    block_id: String,
    #[serde(rename = "bottomLeft")]
    bottom_left: (i32, i32),
    #[serde(rename = "topRight")]
    top_right: (i32, i32),

    color: Option<[u8; 4]>,
    #[serde(rename = "pngBottomLeftPoint")]
    bottom_left_point: Option<(i32, i32)>,
}

#[derive(Clone)]
pub struct Problem {
    pub id: Option<i32>,
    pub width: i32,
    pub height: i32,
    pub initial_img: Option<Image>,
    pub target: Image,
    start_blocks: Vec<(BlockId, Block)>,
    pub base_costs: BaseCosts,
}

impl Problem {
    pub fn load(problem_id: i32) -> Problem {
        let target = Image::load(&project_path(format!("data/problems/{}.png", problem_id)));
        let initial = project_path(format!("data/problems/{}.initial.json", problem_id));

        let mut start_blocks: Vec<(BlockId, Block)> = vec![];

        //let source_img_path = project_path(format!("data/problems/{}.source.png", problem_id));
        let initial_img_path = project_path(format!("data/problems/{}.initial.png", problem_id));
        let initial_img = if initial_img_path.exists() {
            Some(Image::load(&initial_img_path))
        } else {
            None
        };

        if initial.exists() {
            let initial = std::fs::read_to_string(initial).unwrap();
            let initial: InitialCanvas = serde_json::from_str(&initial).unwrap();
            assert_eq!(target.width, initial.width);
            assert_eq!(target.height, initial.height);
            for b in initial.blocks {
                let block_id = BlockId::parse(&b.block_id);
                let shape = Shape {
                    x1: b.bottom_left.0,
                    y1: b.bottom_left.1,
                    x2: b.top_right.0,
                    y2: b.top_right.1,
                };
                let pic = if let Some(color) = b.color {
                    assert!(b.bottom_left_point.is_none());
                    Pic::Unicolor(Color(color))
                } else {
                    assert!(initial_img.is_some());
                    let (x1, y1) = b.bottom_left_point.unwrap();
                    assert_eq!(shape.x1, x1);
                    assert_eq!(shape.y1, y1);
                    Pic::Bitmap(shape)
                };
                start_blocks.push((
                    block_id,
                    Block {
                        shape,
                        pieces: vec![(shape, pic)],
                    },
                ));
            }
        } else {
            let shape = Shape {
                x1: 0,
                y1: 0,
                x2: target.width,
                y2: target.height,
            };
            start_blocks.push((
                BlockId::root(0),
                Block {
                    shape,
                    pieces: vec![(shape, Pic::Unicolor(Color([255, 255, 255, 255])))],
                },
            ));
        }

        let base_costs = if problem_id >= 36 {
            BaseCosts {
                lcut: 2,
                pcut: 3,
                color: 5,
                swap: 3,
                merge: 1,
            }
        } else {
            BaseCosts {
                lcut: 7,
                pcut: 10,
                color: 5,
                swap: 3,
                merge: 1,
            }
        };

        Problem {
            id: Some(problem_id),
            base_costs,
            width: target.width,
            height: target.height,
            target,
            initial_img, // TODO
            start_blocks,
        }
    }

    pub fn shape(&self) -> Shape {
        Shape {
            x1: 0,
            y1: 0,
            x2: self.width,
            y2: self.height,
        }
    }

    pub fn transform(&self, t: &Transformation) -> Problem {
        let new_target = self.target.transform(t);
        let new_width = new_target.width;
        let new_height = new_target.height;
        let new_initial_img = self.initial_img.as_ref().map(|img| img.transform(t));
        let mut new_start_blocks = vec![];
        for (block_id, block) in &self.start_blocks {
            new_start_blocks.push((block_id.clone(), block.transform(t)));
        }
        Problem {
            id: None,
            width: new_width,
            height: new_height,
            target: new_target,
            initial_img: new_initial_img,
            start_blocks: new_start_blocks,
            base_costs: self.base_costs.clone(),
        }
    }

    pub fn cost(&self, base: i64, size: i32) -> i64 {
        (base * (self.width * self.height) as i64 + (size as i64 + 1) / 2) / size as i64
    }
}

crate::entry_point!("render_moves_example", render_moves_example);
fn render_moves_example() {
    let problem = Problem::load(36);

    // let qq = std::fs::read_to_string(format!("data/problems/problem_{}.json", problem_id)).unwrap();

    let moves = vec![
        // Move::ColorMove {
        //     block_id: BlockId::root(0),
        //     color: Color([0, 74, 173, 255]),
        // },
        PCut {
            block_id: BlockId::root(0),
            x: 200,
            y: 200,
        },
        Swap {
            block_id1: BlockId::root(0).child(0),
            block_id2: BlockId::root(0).child(1),
        },
        /*Move::ColorMove {
            block_id: BlockId::root(0).child(3),
            color: Color([128, 128, 128, 255]),
        },*/
    ];
    let mut painter = PainterState::new(&problem);
    for m in &moves {
        eprintln!("{}", m);
        painter.apply_move(m);
    }
    eprintln!();
    eprintln!("cost: {}", painter.cost);

    let img = painter.render();

    let dist = image_distance(&img, &problem.target);
    eprintln!("distance: {}", dist);
    eprintln!("final score: {}", dist.round() as i64 + painter.cost);

    let path = "outputs/render_moves_example.png";
    img.save(&crate::util::project_path(path));
    eprintln!();
    eprintln!("saved to {}", path);
}

#[cfg(test)]
#[test]
fn test_split_merge() {
    let problem = Problem::load(1);
    let mut painter = PainterState::new(&problem);
    painter.apply_move(&LCut {
        block_id: BlockId::root(0),
        orientation: Orientation::Horizontal,
        line_number: 50,
    });
    painter.apply_move(&Merge {
        block_id1: BlockId::root(0).child(0),
        block_id2: BlockId::root(0).child(1),
    });
}

#[cfg(test)]
#[test]
fn test_rollback() {
    let problem = Problem::load(1);
    let mut painter = PainterState::new(&problem);
    let painter_copy = painter.clone();
    painter.apply_move(&LCut {
        block_id: BlockId::root(0),
        orientation: Orientation::Horizontal,
        line_number: 50,
    });
    painter.rollback_move();
    assert_eq!(painter, painter_copy);
    // painter.apply_move(&Merge { block_id1: BlockId::root(0).child(0), block_id2: BlockId::root(0).child(1) });
}

fn check_score(
    problem_id: i32,
    sol: &str,
    expected_cost: i64,
    expected_dist: i64,
    total_score: i64,
) {
    eprintln!("---");
    dbg!(problem_id);

    let problem = Problem::load(problem_id);
    let mut painter = PainterState::new(&problem);
    let moves = Move::parse_many(sol);
    for m in &moves {
        painter.apply_move(m);
    }
    let img = painter.render();
    let dist = image_distance(&img, &problem.target).round() as i64;
    dbg!(painter.cost);
    dbg!(dist);
    dbg!(painter.cost + dist);
    assert_eq!(painter.cost, expected_cost);
    assert_eq!(dist, expected_dist);
    assert_eq!(painter.cost + dist, total_score);
}

#[cfg(test)]
#[test]
// Total score was compared with the official validator.
fn test_score_calculation() {
    // initial canvas with a bitmap
    check_score(36, "", 0, 71523, 71523);
    check_score(36, "cut [0] [120, 170]", 3, 71523, 71526);
    check_score(
        36,
        "
        cut [0] [200, 200]
        swap [0.0] [0.1]
    ",
        15,
        83031,
        83046,
    );
    check_score(
        36,
        "
        cut [0] [200, 200]
        merge [0.0] [0.1]
    ",
        7,
        71523,
        71530,
    );
    check_score(36, "cut [0] [x] [70]", 2, 71523, 71525);
    check_score(36, "color [0] [123, 22, 71, 55]", 5, 192218, 192223);

    // initial canvas
    check_score(29, "color [0] [0, 74, 173, 255]", 500, 117035, 117535);

    check_score(1, "color [0] [20, 50, 60, 90]", 5, 217215, 217220);

    // merge
    check_score(
        1,
        "
        color [0] [20, 50, 60, 90]
        cut [0] [x] [100]
        merge [0.0] [0.1]
    ",
        13,
        217215,
        217228,
    );

    // merge
    check_score(
        1,
        "
        color [0] [20, 50, 60, 90]
        cut [0] [200, 200]
        merge [0.0] [0.1]
    ",
        19,
        217215,
        217234,
    );

    // initial color
    check_score(1, "", 0, 194616, 194616);

    // Arsenij's manual solution
    check_score(
        1,
        "
        color [0] [255, 255, 255, 255]
        cut [0] [320, 80]
        cut [0.2] [x] [360]
        cut [0.0] [y] [40]
        color [0.1] [0, 74, 175, 255]
        color [0.2.1] [0, 74, 175, 255]
        color [0.0.0] [0, 74, 175, 255]
        cut [0.3] [160, 240]
        color [0.0.1] [0, 0, 0, 255]
        color [0.3.0] [0, 0, 0, 255]
        color [0.3.1] [0, 0, 0, 255]
        cut [0.3.3] [80, 320]
        cut [0.3.3.3] [40, 360]
        cut [0.3.3.2] [120, 360]
        cut [0.3.3.0] [40, 280]
        cut [0.3.3.1] [120, 280]
        cut [0.3.2] [240, 320]
        cut [0.3.2.3] [200, 360]
        cut [0.3.2.2] [280, 360]
        cut [0.3.2.0] [200, 280]
        cut [0.3.2.1] [280, 280]
        cut [0.3.0] [80, 160]
        cut [0.3.0.3] [40, 200]
        cut [0.3.0.2] [120, 200]
        cut [0.3.0.0] [40, 120]
        cut [0.3.0.1] [120, 120]
        cut [0.3.1] [240, 160]
        cut [0.3.1.3] [200, 200]
        cut [0.3.1.2] [280, 200]
        cut [0.3.1.0] [200, 120]
        cut [0.3.1.1] [280, 120]
        cut [0.2.0] [y] [240]
        cut [0.2.0.1] [y] [320]
        cut [0.2.0.1.1] [y] [360]
        cut [0.2.0.1.0] [y] [280]
        cut [0.2.0.0] [y] [160]
        cut [0.2.0.0.1] [y] [200]
        cut [0.2.0.0.0] [y] [120]
        cut [0.0.1] [x] [160]
        cut [0.0.1.0] [x] [80]
        cut [0.0.1.0.0] [x] [40]
        cut [0.0.1.0.1] [x] [120]
        cut [0.0.1.1] [x] [240]
        cut [0.0.1.1.0] [x] [200]
        cut [0.0.1.1.1] [x] [280]
        swap [0.3.3.3.2] [0.3.0.3.3]
        swap [0.3.3.3.0] [0.3.0.3.1]
        swap [0.3.3.2.2] [0.3.0.2.3]
        swap [0.3.3.2.0] [0.3.0.2.1]
        swap [0.3.3.0.2] [0.3.0.0.3]
        swap [0.3.3.0.0] [0.3.0.0.1]
        swap [0.3.3.1.2] [0.3.0.1.3]
        swap [0.3.3.1.0] [0.3.0.1.1]
        swap [0.3.2.3.2] [0.3.1.3.3]
        swap [0.3.2.3.0] [0.3.1.3.1]
        swap [0.3.2.2.2] [0.3.1.2.3]
        swap [0.3.2.2.0] [0.3.1.2.1]
        swap [0.3.2.0.2] [0.3.1.0.3]
        swap [0.3.2.0.0] [0.3.1.0.1]
        swap [0.3.2.1.2] [0.3.1.1.3]
        swap [0.3.2.1.0] [0.3.1.1.1]
        swap [0.2.0.1.1.0] [0.0.1.0.0.0]
        swap [0.2.0.0.0.0] [0.0.1.1.1.0]
        swap [0.2.0.1.0.0] [0.0.1.0.1.0]
        swap [0.2.0.0.1.0] [0.0.1.1.0.0]
    ",
        14423,
        20618,
        35041,
    );
}
