package relay

/*
#cgo CFLAGS: -I${SRCDIR}/include
#cgo LDFLAGS: -L${SRCDIR}/build -lrelay
#include "../include/relay.h"
#include <stdlib.h> // For free()
*/
import "C"
import (
	"unsafe"
)

// Peer represents a P2P peer
type Peer struct {
	ptr C.RelayPeer
}

// PeerManager manages a collection of peers
type PeerManager struct {
	ptr C.RelayPeerManager
}

// PeerDiscovery handles peer discovery
type PeerDiscovery struct {
	ptr C.RelayPeerDiscovery
}

// NewPeer creates a new peer
func NewPeer(id, ip string, port int, isServer int) *Peer {
	cID := C.CString(id)
	cIP := C.CString(ip)
	defer C.free(unsafe.Pointer(cID))
	defer C.free(unsafe.Pointer(cIP))
	ptr := C.relay_create_peer(cID, cIP, C.int(port), C.int(isServer))
	if ptr == nil {
		return nil
	}
	return &Peer{ptr: ptr}
}

// SendMessage sends a message to the peer
func (p *Peer) SendMessage(message string) bool {
	cMsg := C.CString(message)
	defer C.free(unsafe.Pointer(cMsg))
	return C.relay_send_message(p.ptr, cMsg) != 0
}

// ReceiveMessage receives a message from the peer
func (p *Peer) ReceiveMessage() string {
	cStr := C.relay_receive_message(p.ptr)
	if cStr == nil {
		return ""
	}
	defer C.free(unsafe.Pointer(cStr))
	return C.GoString(cStr)
}

// Close closes the peer connection
func (p *Peer) Close() {
	C.relay_close_peer(p.ptr)
}

// Destroy frees the peer resources
func (p *Peer) Destroy() {
	C.relay_destroy_peer(p.ptr)
}

// NewPeerManager creates a new peer manager
func NewPeerManager() *PeerManager {
	return &PeerManager{ptr: C.relay_create_peer_manager()}
}

// AddPeer adds a peer to the manager
func (m *PeerManager) AddPeer(p *Peer) {
	C.relay_add_peer(m.ptr, p.ptr)
}

// RelayMessage relays a message between peers
func (m *PeerManager) RelayMessage(sourceId, targetId, message string) bool {
	cSource := C.CString(sourceId)
	cTarget := C.CString(targetId)
	cMsg := C.CString(message)
	defer C.free(unsafe.Pointer(cSource))
	defer C.free(unsafe.Pointer(cTarget))
	defer C.free(unsafe.Pointer(cMsg))
	return C.relay_relay_message(m.ptr, cSource, cTarget, cMsg) != 0
}

// Destroy frees the peer manager
func (m *PeerManager) Destroy() {
	C.relay_destroy_peer_manager(m.ptr)
}

// NewPeerDiscovery creates a new peer discovery instance
func NewPeerDiscovery(multicastIp string, multicastPort int, localIp string) *PeerDiscovery {
	cMulticastIp := C.CString(multicastIp)
	cLocalIp := C.CString(localIp)
	defer C.free(unsafe.Pointer(cMulticastIp))
	defer C.free(unsafe.Pointer(cLocalIp))
	return &PeerDiscovery{ptr: C.relay_create_peer_discovery(cMulticastIp, C.int(multicastPort), cLocalIp)}
}

// Start starts peer discovery
func (d *PeerDiscovery) Start() {
	C.relay_start_discovery(d.ptr)
}

// Stop stops peer discovery
func (d *PeerDiscovery) Stop() {
	C.relay_stop_discovery(d.ptr)
}

// GetDiscoveredPeers returns the list of discovered peers
func (d *PeerDiscovery) GetDiscoveredPeers() []string {
	var count C.int
	cPeers := C.relay_get_discovered_peers(d.ptr, &count)
	if cPeers == nil || count == 0 {
		return nil
	}
	defer C.free(unsafe.Pointer(cPeers))

	peers := make([]string, count)
	for i := 0; i < int(count); i++ {
		cStr := *(**C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(cPeers)) + uintptr(i)*unsafe.Sizeof(*cPeers)))
		peers[i] = C.GoString(cStr)
		C.free(unsafe.Pointer(cStr))
	}
	return peers
}

// Destroy frees the peer discovery resources
func (d *PeerDiscovery) Destroy() {
	C.relay_destroy_peer_discovery(d.ptr)
}
