# Example Application

Demonstrates usage of the Relay P2P library in Go.

## Files
- **`main.go`**:
  - **Purpose**: Demo script creating a server and clients, broadcasting messages.
  - **Functions**:
    - `main()`: Sets up a server peer, creates clients, starts discovery, broadcasts messages, and receives them.
    - (Implicitly uses `relay` package functions like `NewPeer`, `Broadcast`, `ReceiveMessage`).