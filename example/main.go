package main

import (
	"fmt"
	"relay"
	"time"
)

func main() {
	// Start server peer
	server := relay.NewPeer("server_1", "0.0.0.0", 8080, 1)
	if server == nil {
		fmt.Println("Failed to create server peer")
		return
	}
	defer server.Destroy()
	fmt.Println("Server peer created")

	// Give server time to start listening
	time.Sleep(1 * time.Second)

	// Start client peer
	client := relay.NewPeer("peer_1", "127.0.0.1", 8080, 0)
	if client == nil {
		fmt.Println("Failed to create client peer")
		return
	}
	defer client.Destroy()
	fmt.Println("Client peer created")

	// Test sending a message
	if client.SendMessage("Hello from client") {
		fmt.Println("Message sent successfully")
		if msg := server.ReceiveMessage(); msg != "" {
			fmt.Println("Server received:", msg)
		} else {
			fmt.Println("No message received")
		}
	} else {
		fmt.Println("Failed to send message")
	}
}
