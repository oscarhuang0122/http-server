Simple HTTP Server (C, select-based)
====================================

Overview
--------

This project is a lightweight HTTP/1.1 static file server written in C.

It demonstrates low-level system programming concepts, including:
- TCP socket programming
- Non-blocking I/O
- Event-driven architecture using select()
- Static file serving

The server uses select() and non-blocking sockets to monitor multiple
client connections without using threads.

This project is intended as a side project for firmware and embedded
Linux engineering roles.


Key Features
------------
- TCP socket programming (POSIX)
- HTTP/1.1 GET request handling
- Event-driven architecture using 'select()'
- Non-blocking I/O (fcntl + O_NONBLOCK)
- Static file serving (HTML, CSS, JS, images)
- Fixed-size static memory buffer (no malloc in request path)
- Modularized codebase (.c /.h separation)
- MIME type detection
- Simple build system using Makefile


Architecture
------------
This project is split into clear modules:

- main.c
  Entry point. Initializes the server and starts the event loop.

- server.c / server.h
  - Socket creation and configuration
  - Non-blocking server setup
  - Client multiplexing using select()
  - Accepting new connections

- http.c / http.h
  - HTTP request parsing
  - Static file lookup
  - HTTP response construction
  - HTTP error responses (400, 403, 404, 405, and oversized files)

- util.c / util.h
  - Utility functions
  - Non-blocking socket helper
  - MIME type detection

Static files are served from 'www/' directory.


Memory Model
------------
- Uses fixed-size buffers for request handling and file reads
- Avoids dynamic memory allocation in the hot path
- Suitable for embedded or resource-constrained environments


Build Instructions
------------------
Requirements:
- GCC
- Linux or WSL environment

Build:
    make

Run:
    make run

Clean:
    make clean


Usage
-----
1. Place static files in the 'www/' directory
    (e.g., index.html, style.css, script.js)

2. Start the server:
    ./http_server

3. Open a browser and visit:
    http://localhost:8080


Example
-------
Request:
    GET /index.html HTTP/1.1

Response:
    HTTP/1.1 200 OK
    Content-Type: text/html
    Content-Length: ...


Skills Demonstrated
-------------------
- C systems programming
- POSIX networking APIs
- Event-driven server design
- Non-blocking I/O
- Embedded-style memory management
- Code modularization and build tooling


Possible Future Improvements
----------------------------
- HTTP keep-alive support
- Chunked transfer encoding
- epoll-based backend
- Basic routing support
- Logging system
- Unit tests for HTTP parsing

Development History
-------------------

- http_server_prototype.c: Original single-file implementation.
- src/: Refactored modular implementation with separated
  server, HTTP, and utility components.

Author
------
Oscar Huang