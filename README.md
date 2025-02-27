# Relay P2P Library

A Go library with a C++ backend for peer-to-peer networking, featuring peer discovery, messaging, and management. Designed for cross-platform use, it integrates TCP/UDP sockets and multicast discovery.

## Overview
- **Purpose**: Enable P2P communication between Go applications with a high-performance C++ core.
- **Components**:
  - `src/`: C++ source files implementing core functionality.
  - `include/relay/`: C++ headers defining interfaces.
  - `relay.go`: Go wrapper for C++ functions via cgo.
  - `example/`: Demo application (`main.go`).
  - `build/`: Compiled shared library (`librelay.so`).

## Features
- Peer creation (server/client) with TCP messaging.
- Multicast-based peer discovery.
- Peer management with broadcasting.
- Thread-safe logging.

## Building
```bash
g++ -shared -o build/librelay.so src/*.cpp -fPIC
go build -o relay_example example/main.go
LD_LIBRARY_PATH=$PWD/build ./relay_example