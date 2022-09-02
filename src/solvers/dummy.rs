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

crate::entry_point!("dummy_solver", dummy_solver);
fn dummy_solver() {
    let args: Vec<String> = std::env::args().skip(2).collect();
    if args.len() != 1 {
        println!("Usage: dummy_solver <problem id>");
        std::process::exit(1);
    }
    let problem_id: i32 = args[0].parse().unwrap();

    let target = image::open(project_path(format!("data/problems/{}.png", problem_id))).unwrap().to_rgba8();

    // Just take some color from the target image.
    let color = target.get_pixel(0, 0);
    let color = Color { r: color.0[0], g: color.0[1], b: color.0[2], a: color.0[3] };

    let moves = vec![
        Move::Color {
            block_id: vec![0],
            color,
        }
    ];

    let mut painter = PainterState::new(target.width() as i32, target.height() as i32);
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
    img.save(crate::util::project_path(&output_path)).unwrap();
    eprintln!("saved to {}", output_path);
}
