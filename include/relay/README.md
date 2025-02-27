# Header Files

C++ header files defining the Relay P2P library’s interfaces.

## Files
- **`relay.h`**:
  - **Purpose**: C API for Go integration.
  - **Declarations**: All `relay_*` functions (see `src/relay_go.cpp`).

- **`peer.h`**:
  - **Purpose**: Defines the `Peer` class.
  - **Class**: `Peer`
    - Members: `id_`, `ip_`, `port_`, `socket_`, `clients_`.
    - Methods: `sendMessage()`, `receiveMessage()`, `acceptClients()`, `getSocket()`, `getClients()`.

- **`socket_wrapper.h`**:
  - **Purpose**: Defines the `SocketWrapper` class for socket management.
  - **Class**: `SocketWrapper`
    - Methods: `initialize()`, `send()`, `receive()`, `accept()`, `setReceiveTimeout()`.

- **`peer_manager.h`**:
  - **Purpose**: Defines the `PeerManager` class.
  - **Class**: `PeerManager`
    - Methods: `addPeer()`, `broadcast()`, `getPeer()`.

- **`peer_discovery.h`**:
  - **Purpose**: Defines the `PeerDiscovery` class.
  - **Class**: `PeerDiscovery`
    - Methods: `start()`, `stop()`, `getDiscoveredPeers()`.

- **`logger.h`**:
  - **Purpose**: Defines the `Logger` class.
  - **Class**: `Logger`
    - Methods: `getInstance()`, `log()`, `enableFileLogging()`.