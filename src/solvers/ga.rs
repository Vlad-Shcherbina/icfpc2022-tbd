use std::cmp::{max, min};
use fxhash::FxHashSet as HashSet;
use rand::distributions::Uniform;
use crate::basic::*;
// use crate::invocation::{record_this_invocation, Status};
use crate::{color_util, seg_util};
use rand::prelude::*;
use rand::rngs::ThreadRng;
use crate::image::Image;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::{best_solution, upload_solution};

crate::entry_point!("ga_solver", ga_solver);

struct Framework<'a> {
    rng: ThreadRng,
    state: State<'a>,
    uniform_action: UniformAction,
    pop_size: usize,
    pop_multiplier: usize,
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

struct UniformAction {
    uniform_shape: UniformSubshape,
    uniform_two_shapes: UniformTwoNonIntersectingSubshapes,
}

impl UniformAction {
    fn new(shape: Shape) -> UniformAction {
        UniformAction {
            uniform_shape: UniformSubshape::new(shape),
            uniform_two_shapes: UniformTwoNonIntersectingSubshapes::new(shape),
        }
    }
}

impl Distribution<Action> for UniformAction {
    fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> Action {
        if rng.gen_bool(0.5) {
            Action::Color {
                shape: self.uniform_shape.sample(rng),
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

impl<'a> Framework<'a> {
    fn new(problem: &'a Problem, pop_size: usize, pop_multiplier: usize) -> Self {
        let shape = Shape {x1: 0, y1: 0, x2: problem.width, y2: problem.height};
        Framework {
            state: State::new(problem),
            rng: rand::thread_rng(),
            uniform_action: UniformAction::new(shape),
            pop_size,
            pop_multiplier
        }
    }

    fn crossover(&mut self, action1: Actions, action2: Actions) -> Option<Actions> {
        if action1.0.is_empty() || action2.0.is_empty() {
            return None
        }
        let split1 = Uniform::new(0, action1.0.len()).sample(&mut self.rng);
        let split2 = Uniform::new(0, action2.0.len()).sample(&mut self.rng);
        Some(if self.rng.gen_bool(0.5) {
            let mut actions = action1.0.split_at(split1).0.to_vec();
            actions.extend(action2.0.split_at(split2).1.to_vec());
            Actions(actions)
        } else {
            let mut actions = action2.0.split_at(split2).0.to_vec();
            actions.extend(action1.0.split_at(split1).1.to_vec());
            Actions(actions)
        })
    }

    // fn mutate_action(&mut self, action: &mut Action) {
    //     match action {
    //         Action::Color { shape, color } => {
    //         }
    //         Action::Swap { .. } => {}
    //     }
    // }

    fn mutate_actions(&mut self, actions: &mut Actions) {
        loop {
            let mutation = Mutation::ALL.choose(&mut self.rng).unwrap();
            match mutation {
                Mutation::AddNew => {
                    let action = self.random_action();
                    let pos = Uniform::new(0, actions.0.len() + 1).sample(&mut self.rng);
                    actions.0.insert(pos, action);
                    break
                }
                Mutation::Delete => {
                    if actions.0.is_empty() {
                        continue
                    }
                    let pos = Uniform::new(0, actions.0.len()).sample(&mut self.rng);
                    actions.0.remove(pos);
                    break
                }
                Mutation::SmallMove => {
                    if actions.0.is_empty() {
                        continue
                    }
                    let dx = Uniform::new(-10, 10).sample(&mut self.rng);
                    let dy = Uniform::new(-10, 10).sample(&mut self.rng);
                    let root_shape = self.state.problem.shape();
                    let pos = Uniform::new(0, actions.0.len()).sample(&mut self.rng);
                    let a = actions.0[pos].shift(&root_shape, dx, dy);
                    if let Some(a) = a {
                        actions.0[pos] = a;
                        break
                    }
                    continue
                }
                Mutation::SmallResize => {
                    if actions.0.is_empty() {
                        continue
                    }
                    let dx = Uniform::new(-10, 10).sample(&mut self.rng);
                    let dy = Uniform::new(-10, 10).sample(&mut self.rng);
                    let root_shape = self.state.problem.shape();
                    let pos = Uniform::new(0, actions.0.len()).sample(&mut self.rng);
                    let a = actions.0[pos].resize(&root_shape, dx, dy);
                    if let Some(a) = a {
                        actions.0[pos] = a;
                        break
                    }
                    continue
                }
                Mutation::Swap => {
                    if actions.0.len() < 2 {
                        continue
                    }
                    let pos1 = Uniform::new(0, actions.0.len()).sample(&mut self.rng);
                    let pos2 = Uniform::new(0, actions.0.len()).sample(&mut self.rng);
                    if pos1 == pos2 {
                        continue
                    }
                    actions.0.swap(pos1, pos2);
                    break
                }
            }
        }
    }

    fn random_action(&mut self) -> Action {
        self.uniform_action.sample(&mut self.rng)
    }

    // fn random_actions(&mut self) -> Actions {
    //     let mut result = vec![];
    //     for _ in 0..10 {
    //         result.push(self.random_action())
    //     }
    //     Actions(result)
    // }

    fn run_one_generation(&mut self, gen: Vec<Actions>) -> Vec<Actions> {
        let mut next_pop = gen;
        for _ in 0..self.pop_multiplier*self.pop_size {
            let mut a = next_pop.choose(&mut self.rng).unwrap().clone();
            self.mutate_actions(&mut a);
            next_pop.push(a);
        }
        for _ in 0..self.pop_multiplier*self.pop_size {
            let a1 = next_pop.choose(&mut self.rng).unwrap().clone();
            let a2 = next_pop.choose(&mut self.rng).unwrap().clone();
            if let Some(a) = self.crossover(a1, a2) {
                next_pop.push(a);
            }
        }
        for _ in 0..self.pop_multiplier*self.pop_size {
            let mut a = next_pop.choose(&mut self.rng).unwrap().clone();
            for _ in 0..10 {
                self.mutate_actions(&mut a);
            }
            next_pop.push(a);
        }
        let mut results: Vec<(i64, Actions)> = next_pop.into_iter().map(|a| {
            (self.state.eval(&a).0, a)
        }).collect();
        results.sort_by(|(cost1, _actions1), (cost2, _actions2)| {
            cost1.cmp(cost2)
        });
        results.into_iter().map(|(_cost, actions)| {
            actions
        }).take(self.pop_size).collect()
    }

    fn run<F: FnMut(Vec<Move>)>(&mut self, mut callback: F) -> (i64, Vec<Move>) {
        let (mut best, mut best_moves) = self.state.eval(&Actions(vec![]));
        let mut population = vec![Actions(vec![])];
        for gen in 0.. {
            population = self.run_one_generation(population);
            let best_actions = &population[0];
            let (res, moves) = self.state.eval_custom_merge(best_actions, &MergeAllPrecise{});
            println!("GEN {} SCORE {}", gen, res);
            if res < best {
                println!("NEW BEST!");
                best = res;
                best_moves = moves.clone();
                callback(moves);
            }
        }
        (best, best_moves)
    }
}

#[derive(Clone, Debug)]
enum Action {
    Color {
        shape: Shape,
    },
    Swap {
        shape1: Shape, shape2: Shape,
    },
}

impl Action {
    fn resize_shape(root: &Shape, shape: &Shape, dx: i32, dy: i32) -> Option<Shape> {
        let x1 = shape.x1 - dx;
        let x2 = shape.x2 + dx;
        let y1 = shape.y1 - dy;
        let y2 = shape.y2 + dy;
        if x1 >= x2 || y1 >= y2 {
            return None
        }
        if x1 < 0 || y1 < 0 || x2 > root.x2 || y2 > root.y2 {
            return None
        }
        Some(Shape{ x1, y1, x2, y2 })
    }

    fn resize(&self, root: &Shape, dx: i32, dy: i32) -> Option<Action> {
        match self {
            Action::Color { shape } => {
                let shape = Action::resize_shape(root, shape, dx, dy)?;
                Some(Action::Color { shape })
            }
            Action::Swap { shape1, shape2 } => {
                let shape1 = Action::resize_shape(root, shape1, dx, dy)?;
                let shape2 = Action::resize_shape(root, shape2, dx, dy)?;
                if shape1.intersect(&shape2).is_some() {
                    return None
                }
                Some(Action::Swap{
                    shape1,
                    shape2,
                })
            }
        }
    }

    fn shift_shape(root: &Shape, shape: &Shape, dx: i32, dy: i32) -> Option<Shape> {
        let x1 = shape.x1 + dx;
        let x2 = shape.x2 + dx;
        let y1 = shape.y1 + dy;
        let y2 = shape.y2 + dy;
        if x1 < 0 || y1 < 0 || x2 > root.x2 || y2 > root.y2 {
            return None
        }
        Some(Shape{ x1, y1, x2, y2 })
    }

    fn shift(&self, root: &Shape, dx: i32, dy: i32) -> Option<Action> {
        match self {
            Action::Color { shape } => {
                let shape = Action::shift_shape(root, shape, dx, dy)?;
                Some(Action::Color { shape })
            }
            Action::Swap { shape1, shape2 } => {
                let shape1 = Action::shift_shape(root, shape1, dx, dy)?;
                let shape2 = Action::shift_shape(root, shape2, dx, dy)?;
                Some(Action::Swap { shape1, shape2 })
            }
        }
    }
}

#[derive(Clone, Debug)]
struct Actions (pub Vec<Action>);

enum Mutation {
    AddNew,
    Delete,
    SmallMove,
    SmallResize,
    Swap,
}

impl Mutation {
    const ALL: [Mutation; 5] = [
        Mutation::AddNew,
        Mutation::Delete,
        Mutation::SmallMove,
        Mutation::SmallResize,
        Mutation::Swap,
    ];
}


struct State<'a> {
    problem: &'a Problem,
    root_id: BlockId,
    painter: PainterState<'a>,
    initial_colors: HashSet<Color>,
    last_used_color: Color,
}

trait MergeAll {
    fn merge_all(&self, p: &mut PainterState) -> (BlockId, Vec<Move>);
}

struct MergeAllPrecise {}

impl MergeAll for MergeAllPrecise {
    fn merge_all(&self, p: &mut PainterState) -> (BlockId, Vec<Move>) {
        seg_util::merge_all(p)
    }
}

struct MergeAllFast {}

impl MergeAll for MergeAllFast {
    fn merge_all(&self, p: &mut PainterState) -> (BlockId, Vec<Move>) {
        seg_util::merge_all_2(p)
    }
}

impl<'a> State<'a> {
    fn new(problem: &'a Problem) -> State<'a> {
        let mut painter = PainterState::new(problem);
        let (root_id, _) = seg_util::merge_all(&mut painter);
        let initial_colors = HashSet::default();
        State {
            problem,
            root_id,
            painter,
            initial_colors,
            last_used_color: Color::default(),
        }
    }

    fn reeval(&self, moves: &Vec<Move>) -> (i64, Image) {
        let mut painter = PainterState::new(self.problem);
        for mv in moves {
            painter.apply_move(mv);
        }
        (painter.cost, painter.render())
    }

    fn eval(&mut self, actions: &Actions) -> (i64, Vec<Move>) {
        self.eval_custom_merge(actions, &MergeAllFast{})
    }

    fn drop_trailing_non_edits(&self, moves: &mut Vec<Move>) {
        while let Some(mv) = moves.last() {
            let drop = match mv {
                Move::PCut { .. } => { true }
                Move::LCut { .. } => { true }
                Move::ColorMove { .. } => { false }
                Move::Swap { .. } => { false }
                Move::Merge { .. } => { true }
            };
            if drop {
                moves.pop();
            } else {
                break
            }
        }
    }

    fn eval_custom_merge<M: MergeAll>(&mut self, actions: &Actions, merge: &M) -> (i64, Vec<Move>) {
        self.painter.snapshot();
        self.apply(actions, merge);
        let moves = &self.painter.moves;
        let mut moves = color_util::adjust_colors(self.problem, moves);
        self.drop_trailing_non_edits(&mut moves);
        let (painter_cost, painter_image) = self.reeval(&moves);
        self.painter.rollback();
        (painter_cost + image_distance(&painter_image, &self.problem.target) as i64, moves)
    }

    fn gen_unique_color(&mut self) -> Color {
        loop {
            self.last_used_color = self.last_used_color.next();
            if self.initial_colors.contains(&self.last_used_color) {
                continue
            }
            return self.last_used_color
        }
    }

    fn apply<M: MergeAll>(&mut self, actions: &Actions, merge: &M) {
        let mut root_id = self.root_id.clone();
        for action in &actions.0 {
            match action {
                Action::Color { shape } => {
                    let (block_id, _) = seg_util::isolate_rect(&mut self.painter, root_id, *shape);
                    let color = self.gen_unique_color();
                    self.painter.apply_move(&Move::ColorMove { block_id, color });
                    let (new_root, _) = merge.merge_all(&mut self.painter);
                    root_id = new_root;
                }
                Action::Swap { shape1, shape2 } => {
                    let (cut, shape1_idx, shape2_idx);
                    if (shape1.x2 <= shape2.x1 || shape2.x2 <= shape1.x1) && (shape1.y2 <= shape2.y1 || shape2.y2 <= shape1.y1) {
                        let x = min(shape1.x2, shape2.x2);
                        let y = min(shape1.y2, shape2.y2);
                        if shape1.x1 < x && shape1.y1 < y {
                            shape1_idx = 0;
                        } else if shape1.y1 < y {
                            shape1_idx = 1;
                        } else if shape1.x1 < x {
                            shape1_idx = 3;
                        } else {
                            shape1_idx = 2;
                        }
                        shape2_idx = [2, 3, 0, 1][shape1_idx];
                        // println!("CUT {} {} {} {} 1@ {}, 2@ {}", shape1, shape2, x, y, shape1_idx, shape2_idx);
                        cut = Move::PCut { block_id: root_id.clone(), x, y };
                    } else if shape1.x2 <= shape2.x1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Vertical,
                            line_number: shape1.x2,
                        };
                        shape1_idx = 0;
                        shape2_idx = 1;
                    } else if shape2.x2 <= shape1.x1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Vertical,
                            line_number: shape2.x2,
                        };
                        shape1_idx = 1;
                        shape2_idx = 0;
                    } else if shape1.y2 <= shape2.y1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Horizontal,
                            line_number: shape1.y2,
                        };
                        shape1_idx = 0;
                        shape2_idx = 1;
                    } else if shape2.y2 <= shape1.y1 {
                        cut = Move::LCut {
                            block_id: root_id.clone(),
                            orientation: Orientation::Horizontal,
                            line_number: shape2.y2,
                        };
                        shape1_idx = 1;
                        shape2_idx = 0;
                    } else {
                        panic!()
                    }
                    let res = self.painter.apply_move(&cut);
                    let (block_id1, _) = seg_util::isolate_rect(&mut self.painter, res.new_block_ids[shape1_idx].clone(), *shape1);
                    let (block_id2, _) = seg_util::isolate_rect(&mut self.painter, res.new_block_ids[shape2_idx].clone(), *shape2);
                    self.painter.apply_move(&Move::Swap { block_id1, block_id2 });
                    root_id = merge.merge_all(&mut self.painter).0;
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

    let mut client = crate::db::create_client();
    let best = best_solution(&mut client, problem_id).score;
    println!("BEST for {}: {}", problem_id, best);

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

    let new_best_callback = |moves: Vec<Move>| {
        let mut painter = PainterState::new(&problem);
        moves.iter().for_each(|mv| {painter.apply_move(mv);});
        let img = painter.render();
        eprintln!();
        eprintln!("cost: {}", painter.cost);
        let dist = image_distance(&img, &problem.target);
        eprintln!("distance to target: {}", dist);
        let score = dist.round() as i64 + painter.cost;
        eprintln!("final score: {} (target {})", score, best);

        let output_path = format!("outputs/ga_{}.png", problem_id);
        img.save(&crate::util::project_path(&output_path));
        eprintln!("saved to {}", output_path);

        if score < best {
            println!("YAY I WON: {} < {}", score, best);
            let mut tx = client.transaction().unwrap();
            let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
            upload_solution(&mut tx, problem_id, &moves, "ga", &serde_json::Value::Null, incovation_id);
            tx.commit().unwrap();
        }

    };

    let mut framework = Framework::new(&problem, 10, 10);

    framework.run(new_best_callback);

}
