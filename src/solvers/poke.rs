#![allow(clippy::infallible_destructuring_match)]

use crate::basic::*;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;

use std::collections::{HashSet, HashMap};

struct State<'a> {
    painter_state: PainterState<'a>,
    tile_size: i32,
    //initial_palette: HashSet<Color>,
    color_distances: HashMap<(Color, i32, i32), f64>,
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

        let mut initial_palette = HashSet::<Color>::new();
        for block in painter_state.blocks.values() {
            let color = match block.pieces[0].1 {
                Pic::Unicolor(color) => color,
                Pic::Bitmap(_) => todo!(),
            };
            initial_palette.insert(color);
        }
        let mut color_distances = HashMap::<(Color, i32, i32), f64>::new();
        for &color in &initial_palette {
            for i in 0..problem.width/tile_size {
                for j in 0..problem.height/tile_size {
                    let shape = Shape {x1: i*tile_size, y1: j*tile_size, x2: (i+1)*tile_size, y2: (j+1)*tile_size};
                    color_distances.insert((color, i, j), image_slice_distance_to_color(&problem.target, shape, &color));
                }
            }
        }
        State {
            painter_state,
            tile_size,
            //initial_palette,
            color_distances,
            //width: problem.width,
            //height: problem.height
        }
    }

    fn block_idx(&self, block: &Block) -> (i32, i32) {
        (block.shape.x1 / self.tile_size, block.shape.y1 / self.tile_size)
    }
    
    fn solve(&mut self) -> Vec<Move> {
        let mut moves = vec![];
        //let mut cost = 0;
        //let mut block_id = BlockId::root(0);
        //let w_n = self.width / self.tile_size;
        //let h_n = self.height / self.tile_size;
        
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
                let (i1, j1) = self.block_idx(block1);
                let color1 = match block1.pieces[0].1 {
                    Pic::Unicolor(color) => color,
                    Pic::Bitmap(_) => todo!(),
                };
                for (block_id2, block2) in &self.painter_state.blocks {
                    let (i2, j2) = self.block_idx(block2);
                    let color2 = match block2.pieces[0].1 {
                        Pic::Unicolor(color) => color,
                        Pic::Bitmap(_) => todo!(),
                    };
                    let gain_from_swap = swap_cost as f64 +
                        self.color_distances[&(color1, i2, j2)] +
                        self.color_distances[&(color2, i1, j1)] -
                        self.color_distances[&(color1, i1, j1)] -
                        self.color_distances[&(color2, i2, j2)];
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
