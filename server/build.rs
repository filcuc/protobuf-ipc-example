extern crate protoc_rust;

fn main() {
    protoc_rust::run(protoc_rust::Args {
        out_dir: "src/protos",
        input: &["protos/message.proto"],
        includes: &["protos"],
    }).expect("protoc");
}
