# Server
Simple server example with TLS sockets and Protocol Buffers

# Usage
```
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
```

# Certs
For testing purpose you can create your own certs with the
following commands

```
openssl genrsa 2048 > private.pem
openssl req -x509 -new -key private.pem -out public.pem
openssl pkcs12 -export -in public.pem -inkey private.pem -out mycert.pfx
```
