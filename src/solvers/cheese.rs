use crate::util::project_path;
use crate::basic::*;

use crate::image::Image;
use crate::basic::{Color, Move, PainterState, Orientation};
use crate::basic::Move::*;


struct State {
    img: Image,
    moves: Vec<Move>,
}

impl State {
    fn new(img: Image) -> State {
        State {
            img,
            moves: vec![],
        }
    }

    fn get_color(&self, x: i32, y: i32) -> Color {
        self.img.get_pixel(x, y)
        /*let color_rgba = self.img.get_pixel(x as u32, (self.height() - y - 1) as u32);
        Color {
            r: color_rgba.0[0],
            g: color_rgba.0[1],
            b: color_rgba.0[2],
            a: color_rgba.0[3]
        }*/
    }

    fn width(&self) -> i32 {
        self.img.width as i32
    }

    fn height(&self) -> i32 {
        self.img.height as i32
    }

    fn uniform_segment(&self, x: i32, y: i32) -> i32 {
        let color = self.get_color(x, y);
        let mut x_end = x + 1;
        while x_end < self.width() && self.get_color(x_end, y) == color {
            x_end += 1;
        }
        x_end
    }

    fn solve_line(&mut self, y: i32, block_id: &BlockId) {
        let mut x = 0;
        let mut rest_block_id = block_id.clone();
        while x < self.width() {
            let color = self.get_color(x, y);
            let x_end = self.uniform_segment(x, y);
            if x_end < self.width() {
                self.moves.push(LCut {
                    block_id: rest_block_id.clone(),
                    orientation: Orientation::Vertical,
                    line_number: x_end
                });
                self.moves.push(Move::Color { block_id: rest_block_id.child(0), color });
                rest_block_id = rest_block_id.child(1);
            } else {
                self.moves.push(Move::Color { block_id: rest_block_id.clone(), color });
            }
            x = x_end;
        }
    }

    fn split_and_solve_line(&mut self, y: i32, block_id: &BlockId) {
        assert!(y < self.height());
        if y+1 == self.height() {
            self.solve_line(y, block_id);
        } else {
            self.moves.push(LCut {
                block_id: block_id.clone(),
                orientation: Orientation::Horizontal,
                line_number: y+1
            });
            self.solve_line(y, &block_id.child(0));
            self.split_and_solve_line(y+1, &block_id.child(1));
        }
    }

    fn solve(&mut self) {
        let block = BlockId::root(0);
        self.split_and_solve_line(0, &block)
    }

}


crate::entry_point!("cheese_solver", cheese_solver);
fn cheese_solver() {
    let args: Vec<String> = std::env::args().skip(2).collect();
    if args.len() != 1 {
        println!("Usage: cheese_solver <problem id>");
        std::process::exit(1);
    }
    let problem_id: i32 = args[0].parse().unwrap();

    let target = Image::load(&project_path(format!("data/problems/{}.png", problem_id)));
    let mut state = State::new(target.clone());
    state.solve();

    let mut painter = PainterState::new(target.width as i32, target.height as i32);
    for m in &state.moves {
        eprintln!("{}", m);
        painter.apply_move(m);
    }

    let img = painter.render();
    eprintln!();
    eprintln!("cost: {}", painter.cost);
    let dist = image_distance(&img, &target);
    eprintln!("distance to target: {}", dist);
    eprintln!("final score: {}", dist.round() as i64 + painter.cost);

    let output_path = format!("outputs/cheese_{}.png", problem_id);
    img.save(&crate::util::project_path(&output_path));
    eprintln!("saved to {}", output_path);
}
