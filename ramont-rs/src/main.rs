use ramont_rs::Config;
use std::{env, process};

fn main() {
    let config = Config::new(env::args()).unwrap_or_else(|err| {
        eprintln!("failed to parse args: {}", err);
        process::exit(1)
    });

    let server = ramont_rs::Server::new(config);
    server.start()
}
