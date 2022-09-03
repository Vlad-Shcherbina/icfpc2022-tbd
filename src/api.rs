use postgres::Client;
use serde::Deserialize;

use crate::util::DateTime;
use crate::uploader::{best_solution, get_solution};

use multipart::client::lazy::Multipart;

pub struct Conf {
    host_proto: String,
    host_fqdn: String,
    auth: String,
    api_v: String,
    //users: Vec<String>,
    problems: fn(i32) -> Vec<String>,
    submissions: fn(i32) -> Vec<String>,
}

impl Default for Conf {
    fn default() -> Conf {
        Conf {
            host_proto: "https".into(),
            host_fqdn: "robovinci.xyz".into(),
            auth: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJlbWFpbCI6ImptQG1lbW9yaWNpLmRlIiwiZXhwIjoxNjYyMjk2NjEyLCJvcmlnX2lhdCI6MTY2MjIxMDIxMn0.WVHn_sxhVQA0QzSFfHinVNJqhC0LgXme5yvM57O-VgI".into(),
            api_v: "api".into(),
            //users: vec!["users".into()],
            problems: |x| { vec!["problems".into(), format!("{}", x)]},
            submissions: |x| {vec!["submissions".into(), format!("{}", x)]},
        }
    }
}

pub fn with_auth(r: ureq::Request, conf: &Conf) -> ureq::Request {
    r.set("Authorization", ("Bearer ".to_owned() + &conf.auth.clone()).as_str())
}

pub fn url(c: &Conf, path: &[String]) -> String {
    c.host_proto.as_str().to_owned() + "://" +
        c.host_fqdn.as_str() + "/" + c.api_v.as_str() + "/" +
        path.join("/").as_str()
}

pub fn submit_best_solution(client: &mut Client, problem_id: i32) -> i32 {
    let s = best_solution(client, problem_id);
    submit_solution(client, s.id)
}

pub fn submit_solution(client: &mut Client, solution_id: i32) -> i32 {
    let conf = Default::default();
    let s = get_solution(client, solution_id);

    let mut m = Multipart::new();

    m.add_stream(
        "file",
        s.data.as_bytes(),
        Some(format!("tbd{}.isl", s.problem_id)),
        None
    );

    let mdata = m.prepare().unwrap();

    let r = with_auth(
        ureq::post(&url(&conf, &((conf.problems)(s.problem_id)))).
            set("Content-Type", &format!("multipart/form-data; boundary={}", mdata.boundary())),
        &conf
    );
    let y: SubmissionOk = r.send(mdata).unwrap().into_json().unwrap();
    let query = "
    INSERT INTO submissions(problem_id, solution_id, submission_id, timestamp)
    VALUES ($1, $2, $3, NOW())
    RETURNING submission_id
    ";
    client.query_one(query, &[&s.problem_id, &solution_id, &y.submission_id]).unwrap();
    y.submission_id
}

pub fn check_submission(client: &mut Client, submission_id: i32) -> SubmissionStatus {
    let conf = Default::default();
    let r = with_auth(
        ureq::get(&url(&conf, &((conf.submissions)(submission_id)))),
        &conf
    );
    let y: SubmissionStatus = r.call().unwrap().into_json().unwrap();
    let error = y.error.clone();
    let file_url = y.file_url.clone();
    match y.cost {
        None =>
            client.query_opt("
            INSERT INTO bad_submissions(submission_id, error, file_url, timestamp)
            VALUES ($1, $2, $3, NOW())
            ON CONFLICT (submission_id) DO NOTHING
            ", &[
                &submission_id,
                &error.unwrap_or_else( || "no error returned".into()),
                &file_url]
            ).unwrap(),
        Some(cost) =>
            client.query_opt("
            INSERT INTO good_submissions(submission_id, cost, file_url, timestamp)
            VALUES ($1, $2, $3, NOW())
            ON CONFLICT (submission_id) DO NOTHING
            ", &[
                &submission_id,
                &cost,
                &y.file_url]
            ).unwrap()
    };
    y
}

#[derive(Deserialize, Debug)]
pub struct SubmissionOk {
    pub submission_id: i32,
}

#[derive(Deserialize, Debug)]
pub struct SubmissionStatus {
    pub status: String,
    pub cost: Option<i64>,
    pub error: Option<String>,
    pub file_url: String,
}

crate::entry_point!("api_demo", api_demo);
fn api_demo() {
    let mut client = crate::db::create_client();

    let sid = submit_best_solution(&mut client, 24);
    std::thread::sleep(std::time::Duration::from_secs(20));
    println!("{:#?}", check_submission(&mut client, sid));
    client.query("SELECT * FROM good_submissions", &[]).unwrap().into_iter().for_each(|row| {
        let submission_id: i32 = row.get("submission_id");
        let cost: i64 = row.get("cost");
        let timestamp: DateTime = row.get("timestamp");
        println!("* * *\n({:?}) : {:?} @ {:?}", submission_id, cost, timestamp);
    });
}
