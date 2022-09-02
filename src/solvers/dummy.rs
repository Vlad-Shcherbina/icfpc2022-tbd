// Feel free to copy it and do your own stuff.
// Don't call you copy anything like ExampleSolver, DumbSolver, SimpleSolver,
// HeuristicSolver, AStarSolver, or even <your idea>Solver!
// Even if you have one specific easy to describe idea behind your solver,
// over time it will evolve and diverge from this description.
// It's annoying to have GreedySolver that actualy does dynamic programming etc.
// Instead, use memorable and distinctive code names
// (preferably ones that are easy to pronounce too).
// Don't copy this comment when you copy the code.

use crate::util::project_path;
use crate::basic::*;
use crate::image::Image;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;

crate::entry_point!("dummy_solver", dummy_solver);
fn dummy_solver() {
    let args: Vec<String> = std::env::args().skip(2).collect();
    if args.len() != 1 {
        println!("Usage: dummy_solver <problem id>");
        std::process::exit(1);
    }
    let problem_id: i32 = args[0].parse().unwrap();

    let target = Image::load(&project_path(format!("data/problems/{}.png", problem_id)));

    let color = crate::solver_utils::optimal_color_for_block(
        &target, &Shape { x1: 0, y1: 0, x2: target.width, y2: target.height });

    let moves = vec![
        Move::Color {
            block_id: BlockId::root(0),
            color,
        }
    ];

    let mut painter = PainterState::new(target.width, target.height);
    for m in &moves {
        eprintln!("{}", m);
        painter.apply_move(m);
    }

    let img = painter.render();
    eprintln!();
    eprintln!("cost: {}", painter.cost);
    let dist = image_distance(&img, &target);
    eprintln!("distance to target: {}", dist);
    eprintln!("final score: {}", dist.round() as i32 + painter.cost);

    let output_path = format!("outputs/dummy_{}.png", problem_id);
    img.save(&crate::util::project_path(&output_path));
    eprintln!("saved to {}", output_path);

    let mut client = crate::db::create_client();
    let mut tx = client.transaction().unwrap();
    let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
    upload_solution(&mut tx, problem_id, &moves, "dummy", &serde_json::Value::Null, incovation_id);
    tx.commit().unwrap();
}
