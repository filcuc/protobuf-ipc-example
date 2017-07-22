# Protocol Buffer Server-Client example

## Overview
This project shows a simple Client/Server template where
both the Client and Server are written in different languages
and communicate through a secure socket.

This template could be useful as a simple IPC communication scheme
where the client is a Qt Application (UI) and the Server is made
in a different language (in this case Rust). Furthermore the
server can expose different UIs (text or GTK).

For simpler scenario where the IPC communication is only made on
the same machine the SSL requirement can be removed easily.


## Server
See Server/README.md

## Client
See Client/README.md
