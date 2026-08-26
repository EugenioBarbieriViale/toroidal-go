use std::{
    io::{BufReader, prelude::*},
    net::{TcpListener, TcpStream},
};

fn main() {
    let listener = TcpListener::bind("127.0.0.1:8080").unwrap();
    println!("listening on 127.0.0.1:8080");

    for stream in listener.incoming() {
        let stream = stream.unwrap();

        handle_connection(stream);
    }

    println!("shutting down server");
}

fn handle_connection(mut stream: TcpStream) {
    let mut buf_reader = BufReader::new(&stream);

    let mut http_request: Vec<String> = Vec::new();
    let mut content_len = 0usize;

    loop {
        let mut line = String::new();
        buf_reader.read_line(&mut line).unwrap();
        let line = line.trim_end().to_string();

        if line.is_empty() {
            break;
        }

        if let Some(len) = line.strip_prefix("Content-length: ") {
            content_len = len.parse().unwrap_or(0);
        }

        http_request.push(line);
    }

    let mut body = vec![0u8; content_len];
    buf_reader.read_exact(&mut body).unwrap();
    let body = String::from_utf8_lossy(&body);

    println!("Request: {http_request:#?}");
    println!("BODY: {}", body);

    let random_color = rand::random::<u8>() % 2;

    let (status_line, content) = if body == "*!" {
        println!("YAY");
        ("HTTP/1.1 200 OK", format!("{random_color}"))
    } else {
        ("HTTP/1.1 404 NOT FOUND", String::new())
    };

    let len = content.len();
    let response = format!("{status_line}\r\nContent-length: {len}\r\n\r\n{content}");
    println!("{response}");
    stream.write_all(response.as_bytes()).unwrap();
}
