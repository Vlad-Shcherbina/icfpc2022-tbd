use std::cmp::{max, min};
use rand::distributions::Uniform;
use crate::basic::*;
// use crate::invocation::{record_this_invocation, Status};
use crate::seg_util;
// use crate::uploader::upload_solution;
use rand::prelude::*;
use rand::rngs::ThreadRng;

crate::entry_point!("ga_solver", ga_solver);

struct Framework {
    rng: ThreadRng,
    state: State,
    uniform_action: UniformAction,
}

struct UniformSubshape {
    x_distribution: Uniform<i32>,
    y_distribution: Uniform<i32>,
}

impl UniformSubshape {
    fn new(shape: Shape) -> UniformSubshape {
        UniformSubshape {
            x_distribution: Uniform::new_inclusive(shape.x1, shape.x2 - 1),
            y_distribution: Uniform::new_inclusive(shape.y1, shape.y2 - 1),
        }
    }
}

impl Distribution<Shape> for UniformSubshape {
    fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> Shape {
        loop {
            let x1 = self.x_distribution.sample(rng);
            let x2 = self.x_distribution.sample(rng);
            if x1 == x2 {
                continue
            }
            let y1 = self.y_distribution.sample(rng);
            let y2 = self.y_distribution.sample(rng);
            if y1 == y2 {
                continue
            }
            return Shape {
                x1: min(x1, x2),
                x2: max(x1, x2),
                y1: min(y1, y2),
                y2: max(y1, y2),
            }
        }
    }
}

struct UniformTwoNonIntersectingSubshapes {
    shape: Shape,
    subshape_distribution: UniformSubshape,
    x_distribution: Uniform<i32>,
    y_distribution: Uniform<i32>,
}

impl UniformTwoNonIntersectingSubshapes {
    fn new(shape: Shape) -> UniformTwoNonIntersectingSubshapes {
        UniformTwoNonIntersectingSubshapes {
            shape,
            subshape_distribution: UniformSubshape::new(shape),
            x_distribution: Uniform::new_inclusive(shape.x1, shape.x2 - 1),
            y_distribution: Uniform::new_inclusive(shape.y1, shape.y2 - 1),
        }
    }
}

impl Distribution<(Shape, Shape)> for UniformTwoNonIntersectingSubshapes {
    fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> (Shape, Shape) {
        loop {
            let shape = self.subshape_distribution.sample(rng);
            let x1 = self.x_distribution.sample(rng);
            let x2 = x1 + shape.x2 - shape.x1;
            if x2 > self.shape.x2 {
                continue
            }
            let y1 = self.y_distribution.sample(rng);
            let y2 = y1 + shape.y2 - shape.y1;
            if y2 > self.shape.y2 {
                continue
            }
            let shape2 = Shape {x1, x2, y1, y2};
            if shape.intersect(&shape2).is_some() {
                continue
            }
            return (shape, shape2)
        }
    }
}

struct UniformColor {
    uniform_component: Uniform<u8>,
}

impl UniformColor {
    fn new() -> UniformColor {
        UniformColor {
            uniform_component: Uniform::new_inclusive(0, 255),
        }
    }
}

impl Distribution<Color> for UniformColor {
    fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> Color {
        Color ([
            self.uniform_component.sample(rng),
            self.uniform_component.sample(rng),
            self.uniform_component.sample(rng),
            self.uniform_component.sample(rng),
        ])
    }
}

struct UniformAction {
    uniform_shape: UniformSubshape,
    uniform_color: UniformColor,
    uniform_two_shapes: UniformTwoNonIntersectingSubshapes,
}

impl UniformAction {
    fn new(shape: Shape) -> UniformAction {
        UniformAction {
            uniform_shape: UniformSubshape::new(shape),
            uniform_color: UniformColor::new(),
            uniform_two_shapes: UniformTwoNonIntersectingSubshapes::new(shape),
        }
    }
}

impl Distribution<Action> for UniformAction {
    fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> Action {
        if rng.gen_bool(0.5) {
            Action::Color {
                shape: self.uniform_shape.sample(rng),
                color: self.uniform_color.sample(rng),
            }
        } else {
            let (shape1, shape2) = self.uniform_two_shapes.sample(rng);
            Action::Swap {
                shape1,
                shape2
            }
        }
    }
}

impl Framework {
    fn new(problem: Problem) -> Framework {
        let shape = Shape {x1: 0, y1: 0, x2: problem.width, y2: problem.height};
        Framework {
            state: State::new(problem),
            rng: rand::thread_rng(),
            uniform_action: UniformAction::new(shape),
        }
    }

    fn random_actions(&mut self) -> Actions {
        let mut result = vec![];
        for _ in 0..10 {
            result.push(self.uniform_action.sample(&mut self.rng))
        }
        Actions(result)
    }

    fn run(&mut self) -> (i64, Vec<Move>) {
        let (mut best, mut best_moves) = self.state.eval(&Actions(vec![]));
        for _ in 0..10 {
            let actions = self.random_actions();
            let (res, moves) = self.state.eval(&actions);
            println!("SCORE {}", res);
            if res < best {
                best = res;
                best_moves = moves;
            }
        }
        (best, best_moves)
    }
}

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
        let (root_id, _) = seg_util::merge_all_2(&mut painter);
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
        let moves = self.painter.moves.clone();
        let painter_cost = self.painter.cost;
        self.painter.rollback();
        (painter_cost + image_distance(&img, &self.problem.target) as i64, moves)
    }

    fn apply(&mut self, actions: &Actions) {
        let mut root_id = self.root_id.clone();
        for action in &actions.0 {
            match action {
                Action::Color { shape, color } => {
                    let (block_id, _) = seg_util::isolate_rect(&mut self.painter, root_id, *shape);
                    self.painter.apply_move(&Move::ColorMove { block_id, color: *color });
                    let (new_root, _) = seg_util::merge_all_2(&mut self.painter);
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
                    root_id = seg_util::merge_all_2(&mut self.painter).0;
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

    // let mut state = State::new(problem.clone());
    // let (cost, moves) = state.eval(&Actions(vec![
    //     Action::Color {
    //         shape: Shape {x1: 100, y1: 100, x2: 200, y2: 300},
    //         color
    //     },
    //     Action::Swap {
    //         shape1: Shape {x1: 100, y1: 100, x2: 110, y2: 110},
    //         shape2: Shape {x1: 300, y1: 200, x2: 310, y2: 210},
    //     }
    // ]));
    let mut framework = Framework::new(problem.clone());
    let (cost, moves) = framework.run();
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
