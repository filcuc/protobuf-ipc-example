extern crate docopt;
#[macro_use]
extern crate error_chain;
#[macro_use]
extern crate log;
extern crate log4rs;
extern crate native_tls;
extern crate protobuf;

mod client;
mod errors;
mod protos;

use client::Client;
use docopt::Docopt;
use errors::*;
use log::LogLevelFilter;
use log4rs::append::console::ConsoleAppender;
use log4rs::config::{Appender, Config, Root};
use std::io::Read;
use std::fmt::Display;
use std::fs::File;
use std::net::TcpListener;
use std::path::Path;
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
  --bind-host=<hostname>  Bind host address [default: 0.0.0.0].
  --bind-port=<port>      Bind port [default: 56323].
";

/// Initialize the server log
fn init_log() -> Result<log4rs::Handle> {
    let config = Config::builder()
        .appender(Appender::builder().build("stdout", Box::new(ConsoleAppender::builder().build())))
        .build(Root::builder().appender("stdout").build(LogLevelFilter::Info))
        .chain_err(|| "Logging configuration initialization failed")?;
    log4rs::init_config(config).chain_err(|| "Logging initialization failed")
}

#[derive(Debug)]
struct Args {
    certificate_path: String,
    certificate_password: String,
    bind_host_address: String,
    bind_host_port: String
}

/// Parse the command line arguments
fn parse_args() -> Args {
    let args = Docopt::new(USAGE)
        .unwrap_or_else(|e| e.exit())
        .version(Some(VERSION.to_owned()))
        .parse()
        .unwrap_or_else(|e| e.exit());
    Args {
        certificate_path: args.get_str("--certificate").to_owned(),
        certificate_password: args.get_str("--password").to_owned(),
        bind_host_address: args.get_str("--bind-host").to_owned(),
        bind_host_port: args.get_str("--bind-port").to_owned(),
    }
}

/// Load the SSL certificate
fn load_certificate<G, T>(certificate_path: G, certificate_password: T) -> Result<Pkcs12>
    where G: AsRef<Path> + Display, T: AsRef<str> {

    // Open certificate
    let mut file = File::open(&certificate_path)
        .chain_err(|| format!("Could not open certificate file at {}", certificate_path))?;

    // Read certificate
    let mut pkcs12 = vec![];
    file.read_to_end(&mut pkcs12)
        .chain_err(|| format!("Could not read certificate file at {}", certificate_path))?;

    // Decrypt
    Pkcs12::from_der(&pkcs12, certificate_password.as_ref())
        .chain_err(|| format!("Could not decrypt certificate file {}", certificate_path))
}

/// Starts the server accept loop
fn start_server<T>(address: T, port: T, certificate: Pkcs12) -> Result<()> where T: AsRef<str> + Display {
    let address = format!("{}:{}", address, port);

    let listener = TcpListener::bind(&address).chain_err(|| format!("Failed to bind server to {}", address))?;

    let acceptor = {
        let acceptor = TlsAcceptor::builder(certificate)
            .and_then(|acceptor| acceptor.build())
            .chain_err(|| "Failed to create TLS acceptor")?;
        Arc::new(acceptor)
    };

    info!("Server started");

    let mut threads = Vec::new();
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let acceptor = acceptor.clone();
                info!("A client connected");
                let thread = std::thread::spawn(move || {
                    if let Ok(stream) = acceptor.accept(stream) {
                        Client::new(stream).exec();
                    }
                });
                threads.push(thread);
            }
            Err(e) => {
                warn!("Failed to obtain the socket {:?}", e);
            }
        }
    }
    Ok(())
}

fn run() -> Result<()> {
    // Initialize the logger
    let _log_handle: log4rs::Handle = init_log()?;

    // Parse the command line arguments
    let args: Args = parse_args();

    // Load the SSL certificate
    let certificate: Pkcs12 = load_certificate(args.certificate_path, args.certificate_password)?;

    // Start the server
    start_server(args.bind_host_address, args.bind_host_port, certificate)
}

fn main() {
    if let Err(ref e) = run() {
        use std::io::Write;
        let stderr = &mut ::std::io::stderr();
        let errmsg = "Error writing to stderr";

        writeln!(stderr, "error: {}", e).expect(errmsg);

        for e in e.iter().skip(1) {
            writeln!(stderr, "caused by: {}", e).expect(errmsg);
        }

        // The backtrace is not always generated. Try to run this example
        // with `RUST_BACKTRACE=1`.
        if let Some(backtrace) = e.backtrace() {
            writeln!(stderr, "backtrace: {:?}", backtrace).expect(errmsg);
        }

        ::std::process::exit(1);
    }
}
