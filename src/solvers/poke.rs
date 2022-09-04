#![allow(clippy::infallible_destructuring_match)]

use crate::basic::*;
use crate::image::{Image};
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;

use std::collections::{HashMap};

struct State<'a> {
    painter_state: PainterState<'a>,
    //target: Image,
    distances: HashMap<(BlockId, Shape), f64>,
    //width: i32,
    //height: i32
}

impl<'a> State<'a> {
    fn new(problem: &'a Problem) -> State {        
        let painter_state = PainterState::new(problem);
        assert!(painter_state.blocks.len() > 1, "This solver only supports problems with initial blocks");

        let tile_size = painter_state.blocks.iter().next().unwrap().1.shape.width();
        
        for block in painter_state.blocks.values() {
            assert_eq!(block.shape.width(), tile_size);
            assert_eq!(block.shape.height(), tile_size);
        }
        assert_eq!(tile_size * tile_size * painter_state.blocks.len() as i32, problem.width * problem.height);

        let target = problem.target.clone();
        let distances = Self::compute_distances(&painter_state, &target);
        State {
            painter_state,
            //target,
            distances,
            //width: problem.width,
            //height: problem.height
        }
    }

    fn compute_distances(painter_state: &PainterState, target: &Image) -> HashMap<(BlockId, Shape), f64> {
        eprintln!("Computing distances");
        let mut distances = HashMap::<(BlockId, Shape), f64>::new();
        let img = painter_state.render();
        for (block_id1, block1) in &painter_state.blocks {
            for block2 in painter_state.blocks.values() {
                let distance = image_slices_distance(&img, target, block1.shape, block2.shape);
                distances.insert((block_id1.clone(), block2.shape), distance);
            }
        }
        eprintln!("Done computing distances");
        distances
    }
    
    fn solve(&mut self) -> Vec<Move> {
        let mut moves = vec![];
        // Swap cost
        let ApplyMoveResult{cost, new_block_ids: _} =
            self.painter_state.apply_move(&Move::Swap {
                block_id1: BlockId::root(0),
                block_id2:BlockId::root(1)
            });
        let swap_cost = cost;
        self.painter_state.rollback_move();
        
        
        loop {
            let mut best_gain = 0.0;
            let mut best_swap = (BlockId::root(0), BlockId::root(0));

            for (block_id1, block1) in &self.painter_state.blocks {
                for (block_id2, block2) in &self.painter_state.blocks {
                    let gain_from_swap = swap_cost as f64 +
                        self.distances[&(block_id1.clone(), block2.shape)] +
                        self.distances[&(block_id2.clone(), block1.shape)] -
                        self.distances[&(block_id1.clone(), block1.shape)] -
                        self.distances[&(block_id2.clone(), block2.shape)];
                    if gain_from_swap < best_gain {
                        best_gain = gain_from_swap;
                        best_swap = (block_id1.clone(), block_id2.clone());
                    }
                }
            }
            if best_gain == 0.0 {
                break
            } else {
                let m = &Move::Swap {
                    block_id1: best_swap.0,
                    block_id2: best_swap.1
                };
                self.painter_state.apply_move(m);
                moves.push(m.clone());
                //self.update_distances_after_swap(&best_swap.0, &best_swap.1);
            }
        };
        moves
    }
}

crate::entry_point!("poke_solver", poke_solver);
fn poke_solver() {
    let args: Vec<String> = std::env::args().skip(2).collect();
    if args.len() != 1 {
        println!("Usage: poke_solver <problem id>");
        std::process::exit(1);
    }
    let problem_id: i32 = args[0].parse().unwrap();
    let problem = Problem::load(problem_id);
    let mut state = State::new(&problem);

    let moves = state.solve();

    let mut painter = PainterState::new(&problem);
    for m in &moves {
        eprintln!("{}", m);
        painter.apply_move(m);
    }

    let img = painter.render();
    eprintln!();
    eprintln!("cost: {}", painter.cost);
    let dist = image_distance(&img, &problem.target);
    eprintln!("distance to target: {}", dist);
    eprintln!("final score: {}", dist.round() as i64 + painter.cost);

    let output_path = format!("outputs/poke_{}.png", problem_id);
    img.save(&crate::util::project_path(&output_path));
    eprintln!("saved to {}", output_path);

    let mut client = crate::db::create_client();
    let mut tx = client.transaction().unwrap();
    let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
    upload_solution(&mut tx, problem_id, &moves, "poke", &serde_json::Value::Null, incovation_id);
    tx.commit().unwrap();
}
