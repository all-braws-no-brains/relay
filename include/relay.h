#ifndef RELAY_H
#define RELAY_H

#ifdef __cplusplus
extern "C"
{
#endif

    // Opaque handles for C++ objects
    typedef void *RelayPeer;
    typedef void *RelayPeerManager;
    typedef void *RelayPeerDiscovery;

    // Peer functions
    RelayPeer relay_create_peer(const char *id, const char *ip, int port, int isServer);
    int relay_send_message(RelayPeer peer, const char *message);
    const char *relay_receive_message(RelayPeer peer); // Caller must free
    void relay_close_peer(RelayPeer peer);
    void relay_destroy_peer(RelayPeer peer);

    // PeerManager functions
    RelayPeerManager relay_create_peer_manager();
    void relay_add_peer(RelayPeerManager mgr, RelayPeer peer);
    int relay_relay_message(RelayPeerManager mgr, const char *sourceId, const char *targetId, const char *message);
    void relay_destroy_peer_manager(RelayPeerManager mgr);

    // PeerDiscovery functions
    RelayPeerDiscovery relay_create_peer_discovery(const char *multicastIp, int multicastPort, const char *localIp);
    void relay_start_discovery(RelayPeerDiscovery discovery);
    void relay_stop_discovery(RelayPeerDiscovery discovery);
    const char **relay_get_discovered_peers(RelayPeerDiscovery discovery, int *count); // Caller must free
    void relay_destroy_peer_discovery(RelayPeerDiscovery discovery);

#ifdef __cplusplus
}
#endif

#endif // RELAY_H