crate::entry_point!("print_invocations_demo", print_invocations_demo);
fn print_invocations_demo() {
    let mut client = crate::db::create_client();
    for snap in crate::invocation::get_invocations(&mut client) {
        println!("({:?}) {:#?}", snap.status, snap.invocation);
    }
}
