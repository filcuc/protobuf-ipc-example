use protos::message::{Message, Message_Type, GetEventRequest, GetEventReply};
use std::{io, cmp, net};
use std::io::Read;
use protobuf;

#[derive(Debug)]
enum Error {
    IoError(io::Error),
    DecodeError,
    MagicNumberMismatch,
    ProtobufError(protobuf::ProtobufError),
}

impl From<io::Error> for Error {
    fn from(err: io::Error) -> Error { Error::IoError(err) }
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

const EXPECTED_MAGIC_NUMBER: u32 = 12345;

pub struct Client {
    stream: net::TcpStream,
    buffer: [u8; 512],
}

impl Client {
    pub fn new(stream: net::TcpStream) -> Client {
        Client {
            stream: stream,
            buffer: [0; 512]
        }
    }

    pub fn exec(&mut self) {
        loop {
            match self.receive_message() {
                Ok(message) => self.on_message_received(message),
                Err(Error::ProtobufError(_)) => (),
                Err(_) => break
            }
        }
    }

    fn receive_message(&mut self) -> Result<Message, Error> {

        self.stream.read_exact(&mut self.buffer[0..4])?;
        let magic_number = decode_u32(&self.buffer[0..4])?;
        if magic_number != EXPECTED_MAGIC_NUMBER {
            return Err(Error::MagicNumberMismatch);
        };

        self.stream.read_exact(&mut self.buffer[0..4])?;
        let size = decode_u32(&self.buffer[0..4])? as usize;

        let mut message: Vec<u8> = Vec::new();
        message.reserve(size);
        while message.len() < size {
            let num_to_read = cmp::min(self.buffer.len(), size - message.len());
            let (slice, _) = self.buffer.split_at_mut(num_to_read);
            self.stream.read_exact(slice)?;
            message.extend_from_slice(slice);
        }
        let message = protobuf::parse_from_bytes::<Message>(&message)?;
        Ok(message)
    }

    fn on_message_received(&mut self, message: Message) {
        match message.get_field_type() {
            Message_Type::GET_EVENTS_REQUEST => self.on_get_events_request(message.get_getEventRequest()),
            _ => ()
        }
    }

    fn on_get_events_request(&self, request: &GetEventRequest) {
        println!("GetEventsRequest");
        let mut message = Message::new();
        message.set_getEventReply(GetEventReply::new());
    }
}
