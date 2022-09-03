pub fn create_client() -> postgres::Client {
    postgres::Client::connect(
        "postgresql://postgres:stjda743vrs98t32nkdas09@35.195.192.0",
        postgres::NoTls).unwrap()
}

crate::entry_point!("init_tables", init_tables);
fn init_tables() {
    let mut client = create_client();
    crate::invocation::create_table(&mut client);
    client.batch_execute(r#"
    CREATE TABLE IF NOT EXISTS solutions(
        id SERIAL PRIMARY KEY,
        problem_id INTEGER NOT NULL,  -- could be foreign key, but maybe we don't want to bother with the "problems" table

        data TEXT NOT NULL,  -- move instructions as text
        moves_cost BIGINT NOT NULL,
        image_distance BIGINT NOT NULL,

        solver TEXT NOT NULL,  -- name of the solver
        solver_args JSONB NOT NULL,  -- any solver-specific parameters, just to understand what's going on
        invocation_id INTEGER NOT NULL REFERENCES invocations_raw(id),
        timestamp TIMESTAMP WITH TIME ZONE NOT NULL
    );
    "#).unwrap();
    client.batch_execute(r#"
    CREATE TABLE IF NOT EXISTS submissions(

        submission_id INTEGER PRIMARY KEY,

        problem_id INTEGER NOT NULL,  -- could be foreign key, but maybe we don't want to bother with the "problems" table
        solution_id INTEGER NOT NULL REFERENCES solutions(id),

        timestamp TIMESTAMP WITH TIME ZONE NOT NULL
    );

    -- Insert into good_submissions when the API returns a SUCCEEDED in the status field
    CREATE TABLE IF NOT EXISTS good_submissions(

        submission_id INTEGER PRIMARY KEY REFERENCES submissions(submission_id),
        cost BIGINT NOT NULL,
        file_url TEXT NOT NULL,

        timestamp TIMESTAMP WITH TIME ZONE NOT NULL
    );

    -- Insert into bad_submissions when the API returns a FAILED in the status field
    CREATE TABLE IF NOT EXISTS bad_submissions(

        submission_id INTEGER PRIMARY KEY REFERENCES submissions(submission_id),
        error TEXT NOT NULL,
        file_url TEXT NOT NULL,

        timestamp TIMESTAMP WITH TIME ZONE NOT NULL
    );
    "#).unwrap();
}
