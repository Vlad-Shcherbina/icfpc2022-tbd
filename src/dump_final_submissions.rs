use std::collections::BTreeMap;
use std::collections::btree_map::Entry;
use std::fmt::Write;
use crate::util::DateTime;
use crate::util::project_path;
use crate::basic::*;

#[derive(Clone)]
struct Row {
    submitted_timestamp: DateTime,
    found_timestamp: DateTime,
    solution_id: i32,
    moves_cost: i64,
    image_distance: i64,
}

impl Row {
    fn score(&self) -> i64 {
        self.moves_cost + self.image_distance
    }
}

crate::entry_point!("dump_final_submissions", dump_final_submissions);
fn dump_final_submissions() {
    let mut client = crate::db::create_client();
    let rows = client.query("
        SELECT
            sub.timestamp AS submitted_timestamp, sol.timestamp AS found_timestamp,
            sub.problem_id, sol.id, sol.moves_cost, sol.image_distance
        FROM good_submissions AS gsub
        JOIN submissions AS sub ON gsub.submission_id = sub.submission_id
        JOIN solutions AS sol ON sub.solution_id = sol.id
        ORDER BY sub.timestamp
    ", &[]).unwrap();
    let mut best: BTreeMap<i32, Row> = BTreeMap::new();
    for row in rows {
        let submitted_timestamp: std::time::SystemTime = row.get("submitted_timestamp");
        let submitted_timestamp = DateTime::from(submitted_timestamp);
        let found_timestamp: std::time::SystemTime = row.get("found_timestamp");
        let found_timestamp = DateTime::from(found_timestamp);
        let problem_id: i32 = row.get("problem_id");
        let solution_id: i32 = row.get("id");
        let moves_cost: i64 = row.get("moves_cost");
        let image_distance: i64 = row.get("image_distance");

        let r = Row {
            submitted_timestamp,
            found_timestamp,
            solution_id,
            moves_cost,
            image_distance,
        };
        match best.entry(problem_id) {
            Entry::Vacant(v) => {
                v.insert(r);
            }
            Entry::Occupied(mut o) => {
                if r.score() < o.get().score() {
                    o.insert(r);
                }
            }
        }
    }

    let path = project_path("outputs/final_submissions");
    std::fs::create_dir_all(&path).unwrap();
    let mut summary = String::new();
    writeln!(summary, "     score    cost    dist                 found            submitted").unwrap();
    for (problem_id, r) in best {
        let sol: String = client.query_one("
            SELECT data FROM solutions WHERE id = $1
        ", &[&r.solution_id]).unwrap().get("data");

        std::fs::write(path.join(format!("submitted_{:02}.isl", problem_id)), &sol).unwrap();

        let moves = Move::parse_many(&sol);
        let problem = Problem::load(problem_id);
        let mut painter = PainterState::new(&problem);
        for m in &moves {
            painter.apply_move(m);
        }
        painter.render().save(&path.join(format!("rendered_{:02}.png", problem_id)));

        writeln!(summary, "{problem_id:>2}   {:>5} = {:>5} + {:>5}   {}  {}",
            r.score(), r.moves_cost, r.image_distance,
            r.found_timestamp.format("%Y-%m-%d %H:%M:%S"),
            r.submitted_timestamp.format("%Y-%m-%d %H:%M:%S"),
        ).unwrap();
    }
    std::fs::write(path.join("_summary.txt"), &summary).unwrap();
}
