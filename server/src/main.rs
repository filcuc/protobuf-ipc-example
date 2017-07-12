extern crate protobuf;
mod protos;
mod client;
    
use std::net;
use client::{Client};

fn main() {
    let mut threads = Vec::new();
    let listener = net::TcpListener::bind("127.0.0.1:56323").unwrap();
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
