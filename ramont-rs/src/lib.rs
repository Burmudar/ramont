use std::env;
use std::os::unix::net::{UnixListener, UnixStream};

#[derive(Debug)]
pub struct Config {
    socket_path: String,
}

impl Config {
    pub fn new(mut args: env::Args) -> Result<Config, String> {
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

pub fn start_server(config: Config) {
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
