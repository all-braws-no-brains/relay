package main

import (
	"fmt"
	"relay"
	"sync"
	"time"
)

func main() {
	// Start server peer
	server := relay.NewPeer("server_1", "0.0.0.0", 8082, 1)
	if server == nil {
		fmt.Println("Failed to create server peer")
		return
	}
	defer server.Destroy()

	// Create manager peer
	mgr := relay.NewPeerManager()
	defer mgr.Destroy()
	mgr.AddPeer(server)

	clients := []*relay.Peer{}

	for i := 0; i < 3; i++ {
		client := relay.NewPeer(fmt.Sprintf("cleint_%d", i), "127.0.0.1", 8082, 0)
		if client == nil {
			fmt.Println("Failed to create client ", i)
			return
		}
		mgr.AddPeer(client)
		clients = append(clients, client)
	}

	var wg sync.WaitGroup
	ready := make(chan struct{})
	wg.Add(1)
	go func() {
		defer wg.Done()
		server.AcceptClients(len(clients))
		close(ready)
	}()

	select {
	case <-ready:
	case <-time.After(5 * time.Second):
		fmt.Println("Timeout waiting for server to accept clients")
		return
	}

	if mgr.Broadcast("Hello to all peers!") {
		fmt.Println("Broadcast sent to all peers")
	} else {
		fmt.Println("Broadcast failed")
	}

	fmt.Println(len(clients))

	done := make(chan struct{}, len(clients))

	for _, client := range clients {
		wg.Add(1)
		go func(c *relay.Peer) {
			defer wg.Done()
			msg := c.ReceiveMessage()
			fmt.Println(msg)
			if msg != "" {
				fmt.Println("Client --- ", c, " --- received:", msg)
			} else {
				fmt.Println("Client --- ", c, " --- received nothing")
			}
			done <- struct{}{}
		}(client)

	}

	for i := 0; i < len(clients); i++ {
		<-done
	}
	fmt.Println("DEMO complete")

	wg.Wait()
}
