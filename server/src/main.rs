extern crate protobuf;
extern crate docopt;
mod protos;
mod client;
    
use client::Client;
use docopt::Docopt;
use std::net;

const VERSION: &'static str = "0.0.1";

const USAGE: &'static str = "
Server

Usage:
  server [--bind-host=<hostname>] [--bind-port=<port>] run
  server (--help | -h)
  server --version

Options:
  -h --help               Show this screen.
  --version               Show version.
  --bind-host=<hostname>  Bind host address [default: localhost].
  --bind-port=<port>      Bind port [default: 56323].
";

fn main() {
    let args = Docopt::new(USAGE)
        .and_then(|d| d.parse())
        .unwrap_or_else(|e| e.exit());

    if args.get_bool("--version") {
        println!("Server {}", VERSION);
        return;
    }

    if !args.get_bool("run") {
        return;
    }

    let address = args.get_str("--bind-host").to_owned() + ":" + args.get_str("--bind-port");
    println!("Starting server at {}", &address);


    let mut threads = Vec::new();
    let listener = net::TcpListener::bind(&address).unwrap();
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("A client connected");
                let thread = std::thread::spawn(move || {
                    Client::new(stream).exec();
                });
                threads.push(thread);
            },
            Err(e) => {
                println!("Failed to obtain the socket {:?}", e);
            }
        }
    }
}
