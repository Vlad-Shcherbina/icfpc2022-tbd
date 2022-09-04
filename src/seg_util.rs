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

struct State {
    start: std::time::Instant,
    cnt: usize,
    items: Vec<(bool, Shape)>,
    plan: Vec<(usize, usize)>,
    best_plan: Option<Vec<(usize, usize)>>,
    best_cost: i64,
    cost: i64,
    canvas_size: i32,
}

fn plan_merge_all(canvas_size: i32, shapes: &[Shape]) -> Vec<(usize, usize)> {
    // TODO: choose best cost
    let mut state = State {
        start: std::time::Instant::now(),
        cnt: shapes.len(),
        items: shapes.iter().map(|&s| (true, s)).collect(),
        plan: vec![],
        best_plan: None,
        best_cost: 1_000_000_000,
        cost: 0,
        canvas_size,
    };
    fn rec(state: &mut State) {
        if state.start.elapsed() > std::time::Duration::from_secs(1) {
            return;
        }
        if state.cnt == 1 {
            if state.cost < state.best_cost {
                state.best_cost = state.cost;
                state.best_plan = Some(state.plan.clone());
            }
            // state.best_plan = Some(state.plan.clone());
            return;
        }

        for i in 0..state.items.len() {
            if !state.items[i].0 {
                continue;
            }
            for j in i + 1 .. state.items.len() {
                if !state.items[j].0 {
                    continue;
                }
                let merged = merge_shapes(state.items[i].1, state.items[j].1);
                if let Some(merged) = merged {
                    let dcost = (state.canvas_size + (merged.size() + 1) / 2) / merged.size();
                    let dcost = dcost as i64;

                    state.items[i].0 = false;
                    state.items[j].0 = false;
                    state.items.push((true, merged));
                    state.plan.push((i, j));
                    state.cnt -= 1;
                    state.cost += dcost;
                    rec(state);
                    state.cost -= dcost;
                    state.cnt += 1;
                    state.plan.pop();
                    state.items.pop();
                    state.items[i].0 = true;
                    state.items[j].0 = true;
                }
            }
        }
    }
    rec(&mut state);
    state.best_plan.unwrap()
}

pub fn merge_all(p: &mut PainterState) -> (BlockId, Vec<Move>) {
    let _t = crate::stats_timer!("merge_all").time_it();
    let mut shapes: Vec<Shape> = vec![];
    let mut ids: Vec<BlockId> = vec![];
    let mut id_blocks: Vec<_> = p.blocks.iter().collect();
    id_blocks.sort_by_key(|&(id, _)| id.clone());
    for (id, block) in id_blocks {
        shapes.push(block.shape);
        ids.push(id.clone());
    }
    let plan = plan_merge_all(400 * 400, &shapes);  // TODO: don't hardcode canvas size
    let mut moves = vec![];
    for (i, j) in plan {
        let m = Merge {
            block_id1: ids[i].clone(),
            block_id2: ids[j].clone(),
        };
        let ids1 = p.apply_move(&m).new_block_ids;
        moves.push(m);
        ids.push(ids1[0].clone());
    }

    let root_id = ids.pop().unwrap();
    assert_eq!(p.blocks.len(), 1);
    assert!(p.blocks.contains_key(&root_id));
    (root_id, moves)
}


pub fn merge_all_2(p: &mut PainterState) -> (BlockId, Vec<Move>) {
    let _t = crate::stats_timer!("merge_all_2").time_it();
    let mut moves = vec![];
    let mut id = p.blocks.keys().next().unwrap().clone();

    fn next_merge<'a>(items: &mut [(&'a BlockId, &'a Block)]) -> (&'a BlockId, &'a BlockId) {
        items.sort_by(|e1, e2 | {
            e1.1.shape.size().cmp(&e2.1.shape.size())
        });
        for x in items.iter() {
            for y in items.iter() {
                if merge_shapes(x.1.shape, y.1.shape).is_some() {
                    return (x.0, y.0)
                }
            }
        }
        panic!()
    }

    while p.blocks.len() > 1 {
        let mut shapes = vec![];
        for (block_id, block) in &p.blocks {
            shapes.push((block_id, block))
        }
        let (b1, b2) = next_merge(&mut shapes);
        let mv = Merge {
            block_id1: b1.clone(),
            block_id2: b2.clone(),
        };
        let ids = p.apply_move(&mv).new_block_ids;
        moves.push(mv);
        id = ids[0].clone();
    }
    (id, moves)
}

crate::entry_point!("seg_demo", seg_demo);
fn seg_demo() {
    let problem = Problem::load(1);
    let mut painter = PainterState::new(&problem);

    isolate_rect(&mut painter, BlockId::root(0), Shape { x1: 0, y1: 0, x2: 30, y2: 40 });

    merge_all(&mut painter);
}

#[cfg(test)]
fn check_isolate_rect(rect: Shape) {
    let problem = Problem::load(1);
    let mut painter = PainterState::new(&problem);

    let (root_id, _moves) = isolate_rect(&mut painter, BlockId::root(0), rect);
    let root_shape = painter.blocks[&root_id].shape;
    assert_eq!(root_shape, rect);

    merge_all(&mut painter);
    assert_eq!(painter.blocks.len(), 1);
}

#[cfg(test)]
fn check_isolate_rect_2(rect: Shape) {
    let problem = Problem::load(1);
    let mut painter = PainterState::new(&problem);

    let (root_id, _moves) = isolate_rect(&mut painter, BlockId::root(0), rect);
    let root_shape = painter.blocks[&root_id].shape;
    assert_eq!(root_shape, rect);

    merge_all_2(&mut painter);
    assert_eq!(painter.blocks.len(), 1);
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
                    check_isolate_rect_2(Shape { x1, y1, x2, y2 });
                }
            }
        }
    }
}
