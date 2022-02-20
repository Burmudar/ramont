use std::os::unix::net::{UnixListener, UnixStream};
use std::sync::{mpsc, Mutex};
use std::thread::JoinHandle;
use std::{env, thread};

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

#[derive(Debug, PartialEq, Eq)]
enum Event {}

#[derive(Debug, PartialEq, Eq)]
enum Message {
    New,
    Move(Event),
    Terminate,
    Error,
}

struct EventDaemon<'a> {
    thread: &'a Option<JoinHandle<()>>,
    receiver: Mutex<mpsc::Receiver<Message>>,
}

impl<'a> EventDaemon<'a> {
    fn new(receiver: mpsc::Receiver<Message>) -> EventDaemon<'a> {
        EventDaemon {
            receiver: Mutex::new(receiver),
            thread: &None,
        }
    }

    fn start(&'a mut self) {
        self.thread = &'a Some(thread::spawn(|| loop {
            let msg = self.receiver.lock().unwrap().recv().unwrap_or_else(|err| {
                eprintln!("Error receiving message: {}", err);
                Message::Error
            });

            if Message::Terminate == self.handle_message(msg) {
                println!("Shutting down EventDaemon.");
                break;
            }
        }));
    }

    fn handle_message(self, msg: Message) -> Message {
        match msg {
            Message::New => println!("New event src"),
            Message::Move(event) => println!("Event received: {:?}", event),
            _ => println!("Received {:?}", msg),
        };
        msg
    }
}

pub struct Server<'a> {
    config: Config,
    sender: mpsc::Sender<Message>,
    event_daemon: EventDaemon<'a>,
}

impl<'a> Server<'a> {
    pub fn new(config: Config) -> Server<'a> {
        let (sender, receiver) = mpsc::channel();

        Server {
            config,
            sender,
            event_daemon: EventDaemon::new(receiver),
        }
    }

    pub fn start(&self) {
        self.event_daemon.start();
        self.listen();
        println!("Shutting down");
    }

    fn listen(&self) {
        let listener = UnixListener::bind(&self.config.socket_path).unwrap();

        println!("Listening at {}", self.config.socket_path);

        for stream in listener.incoming() {
            match stream {
                Ok(stream) => self.handle_stream(stream),
                Err(err) => println!("Error with incoming stream: {}", err),
            }
        }
    }

    fn handle_stream(&self, stream: UnixStream) {
        println!("Stream connected!")
    }
}
