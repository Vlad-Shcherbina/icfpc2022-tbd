pub fn create_client() -> postgres::Client {
    postgres::Client::connect(
        "postgresql://icfpc2022_playground:tc83ktc83mt@35.188.223.28:8032/icfpc2022_playground",
        postgres::NoTls).unwrap()
}

crate::entry_point!("init_tables", init_tables);
fn init_tables() {
    let mut client = create_client();
    client.batch_execute(r#"
    CREATE TABLE IF NOT EXISTS zzz (
        id SERIAL PRIMARY KEY
    );
    "#).unwrap();

    crate::invocation::create_table(&mut client);
}
