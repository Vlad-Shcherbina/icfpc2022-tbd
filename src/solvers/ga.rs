use crate::basic::*;
// use crate::invocation::{record_this_invocation, Status};
use crate::seg_util;
// use crate::uploader::upload_solution;

crate::entry_point!("ga_solver", ga_solver);

enum Action {
    Color {
        shape: Shape, color: Color,
    },
    Swap {
        shape1: Shape, shape2: Shape,
    },
}

struct Actions (pub Vec<Action>);

struct State {
    problem: Problem,
    root_id: BlockId,
    painter: PainterState,
}

impl State {
    fn new(problem: Problem) -> State {
        let mut painter = PainterState::new(&problem);
        let (root_id, _) = seg_util::merge_all(&mut painter);
        State {
            problem,
            root_id,
            painter,
        }
    }

    fn eval(&mut self, actions: &Actions) -> (i64, Vec<Move>) {
        self.painter.snapshot();
        self.apply(actions);
        let img = self.painter.render();
        println!("{:?}", &self.painter.moves);
        let moves = self.painter.moves.clone();
        self.painter.rollback();
        (self.painter.cost + image_distance(&img, &self.problem.target) as i64, moves)
    }

    fn apply(&mut self, actions: &Actions) {
        let mut root_id = self.root_id.clone();
        for action in &actions.0 {
            match action {
                Action::Color { shape, color } => {
                    let (block_id, _) = seg_util::isolate_rect(&mut self.painter, root_id, *shape);
                    self.painter.apply_move(&Move::ColorMove { block_id, color: *color });
                    let (new_root, _) = seg_util::merge_all(&mut self.painter);
                    root_id = new_root;
                }
                Action::Swap { shape1, shape2 } => {
                    let (cut, s1, s2);
                    // todo PCut when possible
                    if shape1.x2 <= shape2.x1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Vertical,
                            line_number: shape1.x2,
                        };
                        s1 = shape1;
                        s2 = shape2;
                    } else if shape2.x2 <= shape1.x1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Vertical,
                            line_number: shape2.x2,
                        };
                        s1 = shape2;
                        s2 = shape1;
                    } else if shape1.y2 <= shape2.y1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Horizontal,
                            line_number: shape1.y2,
                        };
                        s1 = shape1;
                        s2 = shape2;
                    } else if shape2.y2 <= shape1.y1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Horizontal,
                            line_number: shape2.y2,
                        };
                        s1 = shape2;
                        s2 = shape1;
                    } else {
                        panic!()
                    }
                    let res = self.painter.apply_move(&cut);
                    let (block_id1, _) = seg_util::isolate_rect(&mut self.painter, res.new_block_ids[0].clone(), *s1);
                    let (block_id2, _) = seg_util::isolate_rect(&mut self.painter, res.new_block_ids[1].clone(), *s2);
                    self.painter.apply_move(&Move::Swap { block_id1, block_id2 });
                    root_id = seg_util::merge_all(&mut self.painter).0;
                }
            }
        }
    }
}

fn ga_solver() {
    let args: Vec<String> = std::env::args().skip(2).collect();
    if args.len() != 1 {
        println!("Usage: ga_solver <problem id>");
        std::process::exit(1);
    }
    let problem_id: i32 = args[0].parse().unwrap();
    let problem = Problem::load(problem_id);

    let color = crate::color_util::optimal_color_for_block(
        &problem.target, &Shape { x1: 0, y1: 0, x2: problem.target.width, y2: problem.target.height });

    let mut state = State::new(problem.clone());
    let (cost, moves) = state.eval(&Actions(vec![
        Action::Color {
            shape: Shape {x1: 100, y1: 100, x2: 200, y2: 300},
            color
        },
        Action::Swap {
            shape1: Shape {x1: 100, y1: 100, x2: 110, y2: 110},
            shape2: Shape {x1: 300, y1: 200, x2: 310, y2: 210},
        }
    ]));
    println!("cost {}", cost);
    // assert!(painter.blocks.len() == 1);

    let mut painter = PainterState::new(&problem);
    moves.into_iter().for_each(|mv| {painter.apply_move(&mv);});

    let img = painter.render();
    eprintln!();
    eprintln!("cost: {}", painter.cost);
    let dist = image_distance(&img, &problem.target);
    eprintln!("distance to target: {}", dist);
    eprintln!("final score: {}", dist.round() as i64 + painter.cost);

    let output_path = format!("outputs/ga_{}.png", problem_id);
    img.save(&crate::util::project_path(&output_path));
    eprintln!("saved to {}", output_path);

    // let mut client = crate::db::create_client();
    // let mut tx = client.transaction().unwrap();
    // let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
    // upload_solution(&mut tx, problem_id, &moves, "dummy", &serde_json::Value::Null, incovation_id);
    // tx.commit().unwrap();
}
