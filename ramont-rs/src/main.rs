use std::os::unix::net::{UnixListener, UnixStream};
use std::{env, process};

#[derive(Debug)]
struct Config {
    socket_path: String,
}

impl Config {
    fn new(mut args: env::Args) -> Result<Config, String> {
        args.next(); // get program name out of the way

        let socket_path = match args.next() {
            Some(opt) => {
                if opt.starts_with("-") {
                    return Err(format!("unsupported option: {}", opt));
                }
                opt
            }
            None => return Err(format!("socket path is required")),
        };

        return Ok(Config { socket_path });
    }
}

fn main() {
    let config = Config::new(env::args()).unwrap_or_else(|err| {
        eprintln!("failed to parse args: {}", err);
        process::exit(1)
    });
    start_server(config);
}

fn start_server(config: Config) {
    let listener = UnixListener::bind(&config.socket_path).unwrap();

    println!("Listening at {}", config.socket_path);

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => handle_stream(stream),
            Err(err) => println!("Error with incoming stream: {}", err),
        }
    }

    println!("Shutting down");
}

fn handle_stream(stream: UnixStream) {
    println!("Stream connected!")
}
