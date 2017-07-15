extern crate docopt;
#[macro_use]
extern crate log;
extern crate log4rs;
extern crate native_tls;
extern crate protobuf;

mod client;
mod protos;

use client::Client;
use docopt::Docopt;
use log::LogLevelFilter;
use log4rs::append::console::ConsoleAppender;
use log4rs::append::file::FileAppender;
use log4rs::encode::pattern::PatternEncoder;
use log4rs::config::{Appender, Config, Logger, Root};
use std::{fs, net};
use std::io::Read;
use std::sync::Arc;
use native_tls::{Pkcs12, TlsAcceptor};

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
    let stdout = ConsoleAppender::builder().build();
    let config = Config::builder()
        .appender(Appender::builder().build("stdout", Box::new(stdout)))
        .build(Root::builder().appender("stdout").build(LogLevelFilter::Info))
        .unwrap();
    let handle = log4rs::init_config(config).unwrap();

    let args = Docopt::new(USAGE)
        .and_then(|d| d.parse())
        .unwrap_or_else(|e| e.exit());

    if args.get_bool("--version") {
        info!("Server {}", VERSION);
        return;
    }

    let cert_file_path = args.get_str("--certificate");
    let mut file = match fs::File::open(cert_file_path){
        Ok(f) => f,
        Err(_) => {
            error!("Could not open certificate file at {}", cert_file_path);
            return;
        }
    };
    
    let mut pkcs12 = vec![];
    file.read_to_end(&mut pkcs12).unwrap();
    let pkcs12 = match Pkcs12::from_der(&pkcs12, args.get_str("--password")) {
        Ok(p) => p,
        Err(_) => {
            error!("An error occured opening the certificate");
            return;
        }
    };
    
    let address = args.get_str("--bind-host").to_owned() + ":" + args.get_str("--bind-port");
    info!("Starting server at {}", &address);

    let listener = match net::TcpListener::bind(&address) {
        Ok(l) => l,
        Err(_) => {
            error!("Failed to bind server to {}", address);
            return;
        }
    };

    let acceptor = TlsAcceptor::builder(pkcs12).unwrap().build().unwrap();
    let acceptor = Arc::new(acceptor);
    
    let mut threads = Vec::new();
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let acceptor = acceptor.clone();
                info!("A client connected");
                let thread = std::thread::spawn(move || {
                    let stream = acceptor.accept(stream).unwrap();
                    Client::new(stream).exec();
                });
                threads.push(thread);
            },
            Err(e) => {
                warn!("Failed to obtain the socket {:?}", e);
            }
        }
    }
}
