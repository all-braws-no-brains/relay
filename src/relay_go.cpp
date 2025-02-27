#include "../include/relay.h"
#include "../include/relay/peer.h"
#include "../include/relay/peer_manager.h"
#include "../include/relay/peer_discovery.h"
#include "../include/relay/socket_wrapper.h"
#include <cstring>
#include <vector>

extern "C"
{

    // Peer functions
    RelayPeer relay_create_peer(const char *id, const char *ip, int port, int isServer)
    {
        auto mode = isServer ? relay::SocketMode::TCP_SERVER : relay::SocketMode::TCP_CLIENT;
        auto socket = std::make_shared<relay::SocketWrapper>(mode);
        if (!socket->initialize(ip, port))
        {
            fprintf(stderr, "[ERROR] Failed to initialize %s peer %s at %s:%d\n",
                    isServer ? "server" : "client", id, ip, port);
            return nullptr;
        }
        auto peer = new relay::Peer(id, ip, port, socket);
        if (isServer)
        {
            // For server, we need to call listen()
            if (listen(socket->getSocketFd(), 5) == -1)
            { // 5 is backlog
                fprintf(stderr, "[ERROR] Failed to listen on peer %s: %s\n", id, strerror(errno));
                delete peer;
                return nullptr;
            }
        }
        else
        {
            socket->setReceiveTimeout(2);
        }
        return peer;
    }

    int relay_send_message(RelayPeer peer, const char *message)
    {
        if (!peer || !message)
            return 0;
        return static_cast<relay::Peer *>(peer)->sendMessage(message) ? 1 : 0;
    }

    const char *relay_receive_message(RelayPeer peer)
    {
        if (!peer)
            return nullptr;
        std::string msg = static_cast<relay::Peer *>(peer)->receiveMessage();
        char *result = strdup(msg.c_str()); // Caller must free
        return result;
    }

    void relay_close_peer(RelayPeer peer)
    {
        if (peer)
            static_cast<relay::Peer *>(peer)->closeConnection();
    }

    void relay_destroy_peer(RelayPeer peer)
    {
        delete static_cast<relay::Peer *>(peer);
    }

    void relay_accept_clients(RelayPeer peer, int maxClients)
    {
        if (peer)
            static_cast<relay::Peer *>(peer)->acceptClients(maxClients);
    }

    // PeerManager functions
    RelayPeerManager relay_create_peer_manager()
    {
        return new relay::PeerManager();
    }

    void relay_add_peer(RelayPeerManager mgr, RelayPeer peer)
    {
        if (mgr && peer)
        {
            static_cast<relay::PeerManager *>(mgr)->addPeer(std::shared_ptr<relay::Peer>(static_cast<relay::Peer *>(peer), [](relay::Peer*){}));
        }
    }

    int relay_relay_message(RelayPeerManager mgr, const char *sourceId, const char *targetId, const char *message)
    {
        if (!mgr || !sourceId || !targetId || !message)
            return 0;
        return static_cast<relay::PeerManager *>(mgr)->relayMessage(sourceId, targetId, message) ? 1 : 0;
    }

    void relay_destroy_peer_manager(RelayPeerManager mgr)
    {
        delete static_cast<relay::PeerManager *>(mgr);
    }

    // PeerDiscovery functions
    RelayPeerDiscovery relay_create_peer_discovery(const char *multicastIp, int multicastPort, const char *localIp)
    {
        return new relay::PeerDiscovery(multicastIp, multicastPort, localIp);
    }

    void relay_start_discovery(RelayPeerDiscovery discovery)
    {
        if (discovery)
            static_cast<relay::PeerDiscovery *>(discovery)->start();
    }

    void relay_stop_discovery(RelayPeerDiscovery discovery)
    {
        if (discovery)
            static_cast<relay::PeerDiscovery *>(discovery)->stop();
    }

    const char **relay_get_discovered_peers(RelayPeerDiscovery discovery, int *count)
    {
        if (!discovery || !count)
        {
            *count = 0;
            return nullptr;
        }
        std::vector<std::string> peers = static_cast<relay::PeerDiscovery *>(discovery)->getDiscoveredPeers();
        *count = static_cast<int>(peers.size());
        const char **result = new const char *[*count];
        for (size_t i = 0; i < peers.size(); ++i)
        {
            result[i] = strdup(peers[i].c_str()); // Caller must free each string
        }
        return result; // Caller must free array and strings
    }

    void relay_destroy_peer_discovery(RelayPeerDiscovery discovery)
    {
        delete static_cast<relay::PeerDiscovery *>(discovery);
    }

    int relay_broadcast(RelayPeerManager mgr, const char *message)
    {
        if (!mgr || !message)
        {
            return 0;
        }
        static_cast<relay::PeerManager *>(mgr)->broadcast(std::string(message));
        return 1;
    }
}