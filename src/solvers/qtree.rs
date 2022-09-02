use crate::util::project_path;
use crate::image::Image;
use crate::basic::{BlockId, Color, image_slice_distance, image_distance, Move, PainterState, Shape};
use crate::basic::Move::PCut;
use crate::invocation::{record_this_invocation, Status};
use crate::uploader::upload_solution;

struct State {
    img: Image,
    painter_state: PainterState,
}

impl State {
    fn new(img: Image) -> State {
        let (width, height) = (img.width, img.height);
        State {
            img,
            painter_state: PainterState::new(width, height),
        }
    }

    fn potential_gain(&self, painter_state: &PainterState, shape: Shape) -> i32 {
        let img = painter_state.render();
        let x = shape.x1;
        let y = shape.y1;
        let w = shape.x2 - shape.x1;
        let h = shape.y2 - shape.y1;
        let res =
            image_slice_distance(&self.img, &img, x, y, x, y, w, h) as i32;
        println!("COMPARE {} {} {} {} = {}", w, h, w, h, res);
        res
    }

    fn solve(&mut self) -> (Vec<Move>, i32) {
        let shape = Shape {x1: 0, y1: 0, x2: self.img.width as i32, y2: self.img.height as i32 };
        let block = BlockId::root(0);
        self.qtree(shape, block)
    }

    fn average_color(&self, shape: Shape) -> Color {
        let (mut r, mut g, mut b, mut a) = (0u32, 0u32, 0u32, 0u32);
        let mut pixels = 0;
        for x in shape.x1..shape.x2 {
            for y in shape.y1..shape.y2 {
                let p = self.img.get_pixel(x, y);
                r += p.0[0] as u32; g += p.0[1] as u32; b += p.0[2] as u32; a += p.0[3] as u32;
                pixels += 1;
            }
        }
        Color([
            (r / pixels) as u8,
            (g / pixels) as u8,
            (b / pixels) as u8,
            (a / pixels) as u8,
        ])
    }

    fn qtree_stop_here_recolor_cost(&self, shape: Shape, block_id: BlockId) -> i32 {
        let mut state_copy = self.painter_state.clone();
        let extra_cost = state_copy.apply_move(&Move::Color {
            block_id,
            color: self.average_color(shape),
        });
        extra_cost + self.potential_gain(&state_copy, shape)
    }

    fn qtree_stop_here_no_recolor_cost(&self, shape: Shape) -> i32 {
        self.potential_gain(&self.painter_state, shape)
    }

    fn split_here_cost(&self, shape: Shape, block_id: BlockId) -> i32 {
        let mut state_copy = self.painter_state.clone();
        state_copy.apply_move(&PCut {
            block_id,
            x: (shape.x1 + shape.x2) / 2,
            y: (shape.y1 + shape.y2) / 2,
        })
    }

    fn qtree(&mut self, shape: Shape, block_id: BlockId) -> (Vec<Move>, i32) {
        let split_here = self.split_here_cost(shape, block_id.clone());
        let recolor_cost = self.qtree_stop_here_recolor_cost(shape, block_id.clone());
        let no_recolor_cost = self.qtree_stop_here_no_recolor_cost(shape);

        println!("In {} (aka block {}): repaint {}, no-op: {}, split: {}", shape, block_id, recolor_cost, no_recolor_cost, split_here);
        let (opt_moves, opt) = if no_recolor_cost > recolor_cost {
            (vec![Move::Color { block_id: block_id.clone(), color: self.average_color(shape) }], recolor_cost)
        } else {
            (vec![], no_recolor_cost)
        };

        if split_here > opt {
            println!("Splitting is too expensive at {} (block {}): {} > {}", shape, block_id, split_here, opt);
            return (opt_moves, opt)
        }

        let (x_mid, y_mid) = ((shape.x1 + shape.x2) / 2, (shape.y1 + shape.y2) / 2);
        let subblocks = [
            (0, Shape {x1: shape.x1, y1: shape.y1, x2: x_mid, y2: y_mid}),
            (1, Shape {x1: x_mid, y1: shape.y1, x2: shape.x2, y2: y_mid}),
            (2, Shape {x1: x_mid, y1: y_mid, x2: shape.x2, y2: shape.y2}),
            (3, Shape {x1: shape.x1, y1: y_mid, x2: x_mid, y2: shape.y2}),
        ];
        let mut moves = vec![PCut {
            block_id: block_id.clone(),
            x: x_mid,
            y: y_mid,
        }];
        self.painter_state.apply_move(&moves[0]);
        let mut cost = split_here;
        for (id, subshape) in subblocks {
            let new_block_id = block_id.child(id);
            let (mut new_moves, new_cost) = self.qtree(subshape, new_block_id);
            moves.append(&mut new_moves);
            cost += new_cost;
        }
        if opt < cost {
            (opt_moves, opt)
        } else {
            (moves, cost)
        }
    }
}

crate::entry_point!("qtree_solver", qtree_solver);
fn qtree_solver() {
    let args: Vec<String> = std::env::args().skip(2).collect();
    if args.len() != 1 {
        println!("Usage: qtree_solver <problem id>");
        std::process::exit(1);
    }
    let problem_id: i32 = args[0].parse().unwrap();

    let target = Image::load(&project_path(format!("data/problems/{}.png", problem_id)));

    let mut painter = PainterState::new(target.width, target.height);
    let mut state = State::new(target.clone());
    let (moves, _cost) = state.solve();

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

    let output_path = format!("outputs/qtree_{}.png", problem_id);
    img.save(&crate::util::project_path(&output_path));
    eprintln!("saved to {}", output_path);

    let mut client = crate::db::create_client();
    let mut tx = client.transaction().unwrap();
    let incovation_id = record_this_invocation(&mut tx, Status::Stopped);
    upload_solution(&mut tx, problem_id, &moves, "qtree", &serde_json::Value::Null, incovation_id);
    tx.commit().unwrap();
}