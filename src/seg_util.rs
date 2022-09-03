use crate::basic::*;
use crate::basic::Move::*;
use crate::basic::Orientation::*;

#[allow(clippy::collapsible_else_if)]
pub fn isolate_rect(p: &mut PainterState, mut root_id: BlockId, rect: Shape) -> (BlockId, Vec<Move>) {
    let mut moves = vec![];

    let mut root_shape = p.blocks[&root_id].shape;
    assert!(root_shape.contains(rect));
    if rect.x1 == root_shape.x1 {
        if rect.y1 == root_shape.y1 {
            // do nothing
        } else {
            let m = LCut {
                block_id: root_id.clone(),
                orientation: Horizontal,
                line_number: rect.y1,
            };
            let ids1 = p.apply_move(&m).new_block_ids;
            moves.push(m);
            root_id = ids1[1].clone();
            root_shape = p.blocks[&root_id].shape;
        }
    } else {
        if rect.y1 == root_shape.y1 {
            let m = LCut {
                block_id: root_id.clone(),
                orientation: Vertical,
                line_number: rect.x1,
            };
            let ids1 = p.apply_move(&m).new_block_ids;
            moves.push(m);
            root_id = ids1[1].clone();
            root_shape = p.blocks[&root_id].shape;
        } else {
            let m = PCut {
                block_id: root_id.clone(),
                x: rect.x1,
                y: rect.y1,
            };
            let ids1 = p.apply_move(&m).new_block_ids;
            moves.push(m);
            root_id = ids1[2].clone();
            root_shape = p.blocks[&root_id].shape;
        }
    }
    dbg!(&root_id);
    dbg!(root_shape);

    if rect.x2 == root_shape.x2 {
        if rect.y2 == root_shape.y2 {
            // do nothing
        } else {
            let m = LCut {
                block_id: root_id.clone(),
                orientation: Horizontal,
                line_number: rect.y2,
            };
            let ids1 = p.apply_move(&m).new_block_ids;
            moves.push(m);
            root_id = ids1[0].clone();
            root_shape = p.blocks[&root_id].shape;
        }
    } else {
        if rect.y2 == root_shape.y2 {
            let m = LCut {
                block_id: root_id.clone(),
                orientation: Vertical,
                line_number: rect.x2,
            };
            let ids1 = p.apply_move(&m).new_block_ids;
            moves.push(m);
            root_id = ids1[0].clone();
            root_shape = p.blocks[&root_id].shape;
        } else {
            let m = PCut {
                block_id: root_id.clone(),
                x: rect.x2,
                y: rect.y2,
            };
            let ids1 = p.apply_move(&m).new_block_ids;
            moves.push(m);
            root_id = ids1[0].clone();
            root_shape = p.blocks[&root_id].shape;
        }
    }

    assert_eq!(root_shape, rect);

    (root_id, moves)
}

crate::entry_point!("seg_demo", seg_demo);
fn seg_demo() {
    let problem = Problem::load(1);
    let mut painter = PainterState::new(&problem);

    isolate_rect(&mut painter, BlockId::root(0), Shape { x1: 10, y1: 15, x2: 30, y2: 40 });
}

#[cfg(test)]
fn check_isolate_rect(rect: Shape) {
    let problem = Problem::load(1);
    let mut painter = PainterState::new(&problem);

    let (root_id, _moves) = isolate_rect(&mut painter, BlockId::root(0), rect);
    let root_shape = painter.blocks[&root_id].shape;
    assert_eq!(root_shape, rect);
}

#[cfg(test)]
#[test]
fn test_isolate_rect() {
    let xs = [0, 17, 350, 400];
    let ys = [0, 19, 310, 400];

    for x1 in xs {
        for y1 in ys {
            for x2 in xs {
                if x2 <= x1 {
                    continue;
                }
                for y2 in ys {
                    if y2 <= y1 {
                        continue;
                    }
                    check_isolate_rect(Shape { x1, y1, x2, y2 });
                }
            }
        }
    }
}
