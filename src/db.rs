pub fn create_client() -> postgres::Client {
    postgres::Client::connect(
        "postgresql://icfpc2022_playground:tc83ktc83mt@35.188.223.28:8032/icfpc2022_playground",
        postgres::NoTls).unwrap()
}

crate::entry_point!("init_tables", init_tables);
fn init_tables() {
    let mut client = create_client();
    client.batch_execute(r#"
    CREATE TABLE IF NOT EXISTS solutions(
        id SERIAL PRIMARY KEY,
        problem_id INTEGER NOT NULL,  -- could be foreign key, but maybe we don't want to bother with the "problems" table

        data TEXT NOT NULL,  -- move instructions as text
        moves_cost INTEGER NOT NULL,
        image_distance INTEGER NOT NULL,

        solver TEXT NOT NULL,  -- name of the solver
        solver_args JSONB NOT NULL,  -- any solver-specific parameters, just to understand what's going on
        invocation_id INTEGER NOT NULL REFERENCES invocations_raw(id),
        timestamp TIMESTAMP WITH TIME ZONE NOT NULL
    );
    "#).unwrap();

    crate::invocation::create_table(&mut client);
}
