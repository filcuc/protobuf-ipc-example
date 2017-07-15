extern crate docopt;
extern crate protobuf;
extern crate native_tls;

mod client;
mod protos;

use client::Client;
use docopt::Docopt;
use std::{fs, net};
use std::io::Read;
use std::sync::Arc;
use native_tls::{Pkcs12, TlsAcceptor, TlsStream};

const VERSION: &'static str = "0.0.1";

const USAGE: &'static str = "
Server example with protocol buffers

Usage:
  server [--bind-host=<hostname>] [--bind-port=<port>] --certificate=<crt> --password=<pwd>
  server (--help | -h)
  server --version

Options:
  -h --help               Show this screen.
  --version               Show version.
  --certificate=<crt>     Path to the your .pfx certificate.
  --password=<pwd>        Password for opening the certificate.
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

    let mut file = fs::File::open(args.get_str("--certificate")).unwrap();
    let mut pkcs12 = vec![];
    file.read_to_end(&mut pkcs12).unwrap();
    let pkcs12 = Pkcs12::from_der(&pkcs12, args.get_str("--password")).unwrap();
    
    let address = args.get_str("--bind-host").to_owned() + ":" + args.get_str("--bind-port");
    println!("Starting server at {}", &address);

    let mut threads = Vec::new();
    let listener = net::TcpListener::bind(&address).unwrap();
    let acceptor = TlsAcceptor::builder(pkcs12).unwrap().build().unwrap();
    let acceptor = Arc::new(acceptor);
    
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let acceptor = acceptor.clone();
                println!("A client connected");
                let thread = std::thread::spawn(move || {
                    let stream = acceptor.accept(stream).unwrap();
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
