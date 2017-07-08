extern crate protobuf;
mod protos;

use std::io::Read;
use std::net::{TcpListener, TcpStream};
use protos::message::{Message};

type Result = std::result::Result<(), Error>;

#[derive(Debug)]
enum Error {
    IoError(std::io::Error),
    DecodeError,
    MagicNumberMismatch
}

impl From<std::io::Error> for Error {
    fn from(err: std::io::Error) -> Error {
        Error::IoError(err)
    }
}

fn encode_u32(value: u32, buffer: &mut[u8]) {
    buffer[0] = ((value & 0xFF000000) >> 24) as u8;
    buffer[1] = ((value & 0x00FF0000) >> 16) as u8;
    buffer[2] = ((value & 0x0000FF00) >> 8) as u8;
    buffer[3] = (value & 0x000000FF) as u8;
}

fn decode_u32(buffer: &[u8]) -> std::result::Result<u32, Error> {
    if buffer.len() < 4 {
        return Err(Error::DecodeError);
    }
    let mut result: u32 = buffer[0] as u32;
    result = result << 8;
    result += buffer[1] as u32;
    result = result << 8;
    result += buffer[2] as u32;
    result = result << 8;
    result += buffer[3] as u32;
    Ok(result)
}

fn on_message_received(message: Vec<u8>) {
    let data: &[u8] = &message;
    if let Ok(message) = protobuf::parse_from_bytes::<Message>(data) {
        println!("Message {:?}", message);
    }
}

fn handle_client(mut stream: TcpStream) -> Result {
    const BUFFER_SIZE: usize = 512;
    const EXPECTED_MAGIC_NUMBER: u32 = 12345;
    let mut buffer: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];
    loop {
        stream.read_exact(&mut buffer[0..4])?;
        let magic_number = decode_u32(&buffer[0..4])?;
        if magic_number != EXPECTED_MAGIC_NUMBER {
            return Err(Error::MagicNumberMismatch);
        };

        stream.read_exact(&mut buffer[0..4])?;
        let size = decode_u32(&buffer[0..4])? as usize;

        let mut message: Vec<u8> = Vec::new();
        message.reserve(size);
        while message.len() < size {
            let num_to_read = std::cmp::min(BUFFER_SIZE, size - message.len());
            let (slice, _) = buffer.split_at_mut(num_to_read);
            stream.read_exact(slice)?;
            message.extend_from_slice(slice);
        }
        on_message_received(message);
    }
}

fn main() {
    let mut threads = Vec::new();
    let listener = TcpListener::bind("127.0.0.1:56323").unwrap();
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("A client connected");
                let thread = std::thread::spawn(move || {
                    if let Err(e) =  handle_client(stream) {
                        println!("Client failed with error {:?}", e);
                    }
                });
                threads.push(thread);
            },
            Err(e) => {
                println!("Failed to obtain the socket {:?}", e);
            }
        }
    }
}
