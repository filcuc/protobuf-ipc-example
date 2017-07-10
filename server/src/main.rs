extern crate protobuf;
mod protos;

use std::result::Result;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use protos::message::{Message, Message_Type, GetEventRequest, GetEventReply};

#[derive(Debug)]
enum Error {
    IoError(std::io::Error),
    DecodeError,
    MagicNumberMismatch,
    ProtobufError(protobuf::ProtobufError),
}

impl From<std::io::Error> for Error {
    fn from(err: std::io::Error) -> Error { Error::IoError(err) }
}

impl From<protobuf::ProtobufError> for Error {
    fn from(err: protobuf::ProtobufError) -> Error { Error::ProtobufError(err) }
}

fn encode_u32(value: u32, buffer: &mut[u8]) {
    buffer[0] = ((value & 0xFF000000) >> 24) as u8;
    buffer[1] = ((value & 0x00FF0000) >> 16) as u8;
    buffer[2] = ((value & 0x0000FF00) >> 8) as u8;
    buffer[3] = (value & 0x000000FF) as u8;
}

fn decode_u32(buffer: &[u8]) -> Result<u32, Error> {
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

fn on_get_events_request(stream: &mut TcpStream, request: &GetEventRequest) {
    let mut message = Message::new();
    message.set_getEventReply(GetEventReply::new());
}

fn on_message_received(stream: &mut TcpStream, message: Message) {
    match message.get_field_type() {
        Message_Type::GET_EVENTS_REQUEST => on_get_events_request(stream, message.get_getEventRequest()),
        _ => ()
    }
}

fn receive_message(buffer: &mut[u8], stream: &mut TcpStream) -> Result<Message, Error> {
    const EXPECTED_MAGIC_NUMBER: u32 = 12345;

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
        let num_to_read = std::cmp::min(buffer.len(), size - message.len());
        let (slice, _) = buffer.split_at_mut(num_to_read);
        stream.read_exact(slice)?;
        message.extend_from_slice(slice);
    }
    let message = protobuf::parse_from_bytes::<Message>(&message)?;
    Ok(message)
}

fn handle_client(mut stream: TcpStream) -> Result<(), Error> {
    const BUFFER_SIZE: usize = 512;
    let mut buffer: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];
    loop {
        let message = receive_message(&mut buffer, &mut stream)?;
        on_message_received(&mut stream, message);
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
