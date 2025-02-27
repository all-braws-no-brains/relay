
#### `src/README.md`
```markdown
# Source Files

C++ implementation files for the Relay P2P library.

## Files
- **`relay_go.cpp`**:
  - **Purpose**: C interface between Go and C++ via cgo.
  - **Functions**:
    - `relay_create_peer(id, ip, port, isServer)`: Creates a `Peer` (server or client).
    - `relay_send_message(peer, message)`: Sends a message to a peer.
    - `relay_receive_message(peer)`: Receives a message from a peer.
    - `relay_close_peer(peer)`: Closes a peer’s connection.
    - `relay_destroy_peer(peer)`: Frees a peer.
    - `relay_create_peer_manager()`: Creates a `PeerManager`.
    - `relay_add_peer(mgr, peer)`: Adds a peer to the manager.
    - `relay_broadcast(mgr, message)`: Broadcasts a message to all peers.
    - `relay_destroy_peer_manager(mgr)`: Frees a `PeerManager`.
    - `relay_create_peer_discovery(multicastIp, multicastPort, localIp)`: Starts discovery.
    - `relay_start_discovery(discovery)`: Begins peer discovery.
    - `relay_stop_discovery(discovery)`: Stops discovery.
    - `relay_get_discovered_peers(discovery, count)`: Gets discovered peers.
    - `relay_destroy_peer_discovery(discovery)`: Frees discovery resources.

- **`peer.cpp`**:
  - **Purpose**: Implements the `Peer` class for network peers.
  - **Functions**: 
    - Constructor, `getId()`, `sendMessage()`, `receiveMessage()`, `acceptClients()`, `getSocket()`, `getClients()` (see `peer.h`).

- **`socket_wrapper.cpp`**:
  - **Purpose**: Manages TCP/UDP sockets with abstraction.
  - **Functions**: 
    - Constructor, `initialize()`, `send()`, `receive()`, `accept()`, `close()`, `setReceiveTimeout()` (see `socket_wrapper.h`).

- **`peer_manager.cpp`**:
  - **Purpose**: Manages a collection of peers.
  - **Functions**: 
    - `addPeer()`, `broadcast()`, `getPeer()` (see `peer_manager.h`).

- **`peer_discovery.cpp`**:
  - **Purpose**: Handles multicast peer discovery.
  - **Functions**: 
    - Constructor, `start()`, `stop()`, `getDiscoveredPeers()` (see `peer_discovery.h`).

- **`logger.cpp`**:
  - **Purpose**: Thread-safe logging to console/files.
  - **Functions**: 
    - `getInstance()`, `log()`, `enableFileLogging()` (see `logger.h`).