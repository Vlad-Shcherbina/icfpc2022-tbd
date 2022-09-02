use std::{path::{Path, Component}};
use postgres::{Client, Transaction, types::Json};
use once_cell::sync::{Lazy, OnceCell};

use crate::util::{DateTime};

crate::entry_point!("invocation_demo", invocation_demo);
fn invocation_demo() {
    let mut client = crate::db::create_client();

    let mut tx = client.transaction().unwrap();
    let id = record_this_invocation(&mut tx, Status::KeepRunning { seconds: 35.0 });
    tx.commit().unwrap();
    eprintln!("running as invocation {}...", id);

    std::thread::sleep(std::time::Duration::from_secs(30));

    let mut tx = client.transaction().unwrap();
    record_this_invocation(&mut tx, Status::Stopped);
    tx.commit().unwrap();
    eprintln!("stopped");
}

#[derive(Debug)]
#[derive(serde::Serialize, serde::Deserialize)]
pub struct Invocation {
    pub argv: Vec<String>,
    pub profile: String,  // "debug" or "release"
    pub user: String,
    pub machine: String,
    pub distro: String,
    pub version: Version,
}

#[derive(Debug)]
#[derive(serde::Serialize, serde::Deserialize)]
pub struct Version {
    pub commit: String,
    pub commit_number: i32,
    pub diff_stat: String,
}

pub static THIS_INVOCATION: Lazy<Invocation> = Lazy::new(Invocation::new);
pub static THIS_INVOCATION_ID: OnceCell<i32> = OnceCell::new();

#[derive(Debug)]
pub enum Status {
    KeepRunning { seconds: f64 },  // time to next planned call to record_this_invocation()
    Stopped,
}

#[derive(Debug)]
pub struct Snapshot {
    pub sql_id: i32,
    pub status: RecordedStatus,
    pub invocation: Invocation,
}

#[derive(Debug)]
pub enum RecordedStatus {
    Running { start: DateTime, upcoming: DateTime },
    Stopped { start: DateTime, finish: DateTime },
    Lost { start: DateTime, lost: DateTime },
}

/// Creates or updates current invocation.
/// Returns invocation ID, it will be the same if called mutliple times.
pub fn record_this_invocation(t: &mut Transaction, status: Status) -> i32 {
    let (delta_time, status) = match status {
        Status::KeepRunning { seconds } => (seconds, "RUN"),
        Status::Stopped => (0.0, "STOPPED"),
    };
    let mut inserted = false;
    let id = *THIS_INVOCATION_ID.get_or_init(|| {
        inserted = true;
        let row = t.query_one("
            INSERT INTO invocations_raw(start_time, status, update_time, data)
            VALUES(NOW(), $1, NOW() + $2 * interval '1 second', $3) RETURNING id
            ", &[&status, &delta_time, &Json(&*THIS_INVOCATION)]).unwrap();
        let id: i32 = row.get(0);
        id
    });
    if !inserted {
        t.execute("
            UPDATE invocations_raw SET
                status = $1,
                update_time = NOW() + $2 * interval '1 second',
                data = $3
            WHERE id = $4
        ", &[&status, &delta_time, &Json(&*THIS_INVOCATION), &id]).unwrap();
    }
    id
}

pub fn get_invocations(client: &mut Client) -> Vec<Snapshot> {
    let _do = create_table(client); // update VIEW with the latest LOST entries
    let v = client.query("SELECT id, status, start_time, update_time, data FROM invocations", &[]).unwrap();
    return v.iter().map(|row| {
        let sql_id: i32 = row.get(0);
        let raw_status: &str = row.get(1);
        println!("{}", raw_status);
        let t_0: DateTime = row.get(2);
        let t_u: DateTime = row.get(3);
        let invocation: Json<Invocation> = row.get(4);
        let status = match raw_status {
            "RUN" => RecordedStatus::Running { start: t_0, upcoming: t_u },
            "STOPPED" => RecordedStatus::Stopped { start: t_0, finish: t_u },
            _ => RecordedStatus::Lost { start: t_0, lost: t_u }
        };
        return Snapshot{
            sql_id,
            status,
            invocation: invocation.0,
        }
    }).collect();
}

pub fn create_table(client: &mut Client) {
    client.batch_execute("
    CREATE TABLE IF NOT EXISTS invocations_raw(
        id SERIAL PRIMARY KEY,

        status TEXT NOT NULL,
        -- 'RUN'
        -- 'STOPPED'

        start_time TIMESTAMP WITH TIME ZONE NOT NULL,

        update_time TIMESTAMP WITH TIME ZONE NOT NULL,
        -- If status is 'STOPPED', termination time.
        -- If status is 'RUN', time of the next planned update.
        -- So if status == 'RUN' and update_time < NOW(),
        -- we can conclude that the invocation terminated abnormally
        -- without updating the DB.
        -- Let's call it 'LOST' (even though such status will never appear
        -- in the DB explicitly).

        data JSONB NOT NULL
    );

    CREATE OR REPLACE VIEW invocations AS
    SELECT
        id,
        CASE
            WHEN status = 'RUN' AND update_time < NOW() THEN 'LOST'
            ELSE status
        END AS status,
        start_time,
        update_time,
        data
    FROM invocations_raw;
    ").unwrap();
}

impl Invocation {
    fn new() -> Invocation {
        let mut argv: Vec<String> = std::env::args().collect();

        let path = Path::new(&argv[0]).canonicalize().unwrap();
        let components: Vec<_> = path.components().collect();
        let profile = match &components[..] {
            [..,
             Component::Normal(target),
             Component::Normal(profile),
             Component::Normal(_exe),
            ] => {
                let target = target.to_string_lossy().into_owned();
                let profile = profile.to_string_lossy().into_owned();
                let executable = path.file_stem().unwrap().to_string_lossy().into_owned();
                assert_eq!(target, "target");
                assert!(profile == "debug" || profile == "release");
                argv[0] = executable;
                profile
            },
            _ => panic!("{:?}", components),
        };

        Invocation {
            argv,
            profile,
            user: whoami::username(),
            machine: whoami::hostname(),
            distro: whoami::distro(),
            version: Version::new(),
        }
    }
}

impl Version {
    fn new() -> Version {
        // to refresh diff cache
        std::process::Command::new("git").arg("status")
            .output().unwrap();

        let diff_stat = std::process::Command::new("git")
            .args(&["diff", "HEAD", "--stat"])
            .output().unwrap().stdout;
        let diff_stat = std::str::from_utf8(&diff_stat).unwrap().trim_end().to_owned();

        let commit = std::process::Command::new("git")
            .args(&["rev-parse", "HEAD"])
            .output().unwrap().stdout;
        let commit = std::str::from_utf8(&commit).unwrap().trim_end().to_owned();

        let commit_number = std::process::Command::new("git")
            .args(&["rev-list", "--count", "HEAD"])
            .output().unwrap().stdout;
        let commit_number = std::str::from_utf8(&commit_number).unwrap().trim_end();
        let commit_number = commit_number.parse().unwrap();

        Version {
            commit,
            commit_number,
            diff_stat,
        }
    }
}
