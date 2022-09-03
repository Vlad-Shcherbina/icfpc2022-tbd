pub struct Conf {
    host_proto: String,
    host_fqdn: String,
    auth: String,
    api_v: String,
    users: Vec<String>,
}

impl Default for Conf {
    fn default() -> Conf {
        Conf {
            host_proto: "https".into(),
            host_fqdn: "robovinci.xyz".into(),
            auth: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJlbWFpbCI6ImptQG1lbW9yaWNpLmRlIiwiZXhwIjoxNjYyMjk2NjEyLCJvcmlnX2lhdCI6MTY2MjIxMDIxMn0.WVHn_sxhVQA0QzSFfHinVNJqhC0LgXme5yvM57O-VgI".into(),
            api_v: "api".into(),
            users: vec!["users".into()],
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

crate::entry_point!("api_demo", api_demo);
fn api_demo() {
    let conf: Conf = Default::default();
    let r = ureq::get(&url(&conf, &conf.users));
    let r1 = with_auth(r, &conf);
    let resp = r1.call().unwrap();
    println!("{}", resp.into_string().unwrap())
}
