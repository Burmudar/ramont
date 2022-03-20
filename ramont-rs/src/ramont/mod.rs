pub mod device;

use std::io::{BufRead, BufReader};
use std::os::unix::net::{UnixListener, UnixStream};
use std::sync::{mpsc, Arc, Mutex};
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

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
enum Event {}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
enum Message {
    New,
    Move(Event),
    Terminate,
    Error(mpsc::RecvError),
}

type MessageHandler = fn(msg: Message) -> Option<Message>;
fn handle_message(msg: Message) -> Option<Message> {
    match msg {
        Message::New => println!("New event src"),
        Message::Move(event) => println!("Event received: {:?}", event),
        Message::Terminate => return Some(msg),
        Message::Error(err) => eprintln!("Error message: {}", err),
    };
    None
}

struct EventDaemon {
    sender: mpsc::Sender<Message>,
    receiver: Arc<Mutex<mpsc::Receiver<Message>>>,
    handler: Arc<MessageHandler>,
    worker: Option<JoinHandle<()>>,
}

impl EventDaemon {
    fn new(handler: MessageHandler) -> EventDaemon {
        let (sender, receiver) = mpsc::channel();
        let receiver = Arc::new(Mutex::new(receiver));
        let handler = Arc::new(handler);

        EventDaemon {
            sender,
            receiver,
            handler,
            worker: None,
        }
    }

    fn start(&mut self) {
        // Important to clone the vars that will be shared across the thread boundary
        // Clone increases the reference count for the Arcs Handler and Receiver
        let handler = self.handler.clone();
        let receiver = self.receiver.clone();

        // Move lets the closure take ownsership of surrouding variables
        let thread_handle = thread::spawn(move || loop {
            let msg = receiver.lock().unwrap().recv().unwrap_or_else(|err| {
                eprintln!("Error receiving message: {}", err);
                Message::Error(err)
            });

            // We cannot use self here, since that would mean self has to be shared
            // outside of the thread scope, which we don't want
            //
            // -------- Main Thread -------------
            // |                                |
            // |  self  ------ Inner Thread ----|
            // |        |                      ||
            // |        |       self           ||
            // |        |                      ||
            // |        |______________________||
            // |________________________________
            //
            // As per above, if we use self inside the inner thread we would
            // then have to clone it or do SOME OTHER MAGIC
            //
            // This is why MessageHandler has been created. As it has no shared
            // state with self

            if Message::Terminate == handler(msg).unwrap() {
                println!("Shutting down EventDaemon.");
                break;
            }
        });

        self.worker = Some(thread_handle);
    }
}

fn rm_if_exists(path: &String) {
    if let Some(_) = std::path::Path::new(path)
        .exists()
        .then(|| std::fs::remove_file(path))
    {
        eprintln!("Socket deleted: {}", path);
    } else {
        eprintln!("Failed to delete Socket: {}", path);
    }
}

pub struct Server {
    config: Config,
    event_daemon: EventDaemon,
}

impl Server {
    pub fn new(config: Config) -> Server {
        Server {
            config,
            event_daemon: EventDaemon::new(handle_message),
        }
    }

    pub fn start(&mut self) {
        self.event_daemon.start();
        self.listen();
        println!("Shutting down");
    }

    fn listen(&self) {
        rm_if_exists(&self.config.socket_path);
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
        println!("Stream connected");
        let reader = BufReader::new(&stream);
        for line in reader.lines() {
            match line {
                Ok(v) => println!("Received: {}", v),
                Err(err) => eprintln!("Stream error: {}", err),
            }
        }
        println!("Stream disconnected");
    }
}
