#ifndef RELAY_PEER_MANAGER_H
#define RELAY_PEER_MANAGER_H

#include "peer.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace relay
{

    /**
     * @class PeerManager
     * @brief Manages a collection of peers in the P2P network.
     *
     * The PeerManager class provides functionality to add, remove, and manage peers.
     * It also supports relaying messages between peers and retrieving peer information.
     * This class is thread-safe and can handle concurrent access.
     */
    class PeerManager
    {
    public:
        /**
         * @brief Constructor for the PeerManager.
         *
         * Initializes an empty peer manager.
         */
        PeerManager();

        /**
         * @brief Destructor for the PeerManager.
         */
        ~PeerManager();

        /**
         * @brief Adds a new peer to the manager.
         *
         * @param peer A shared pointer to the peer to be added.
         */
        void addPeer(const std::shared_ptr<Peer> &peer);

        /**
         * @brief Removes a peer from the manager by its ID.
         *
         * @param peerId The unique identifier of the peer to be removed.
         * @return true if the peer was successfully removed, false otherwise.
         */
        bool removePeer(const std::string &peerId);

        /**
         * @brief Checks if a peer exists in the manager.
         *
         * @param peerId The unique identifier of the peer to check.
         * @return True if the peer exists, false otherwise.
         */
        bool hasPeer(const std::string &peerId) const;

        /**
         * @brief Retrieves a peer by its ID.
         *
         * @param peerId The unique identifier of the peer to retrieve.
         * @return A shared pointer to the peer, or nullptr if not found.
         */
        std::shared_ptr<Peer> getPeer(const std::string &peerId) const;

        /**
         * @brief Relays a message from one peer to another.
         *
         * @param sourceId The unique identifier of the source peer.
         * @param targetId The unique identifier of the target peer.
         * @param message The message to be relayed.
         * @return true if the message was successfully relayed, false otherwise.
         */
        bool relayMessage(const std::string &sourceId, const std::string &targetId, const std::string_view message);

        /**
         * @brief Adds a list of discovered peers to the manager.
         *
         * @param discoveredPeers A vector of shared pointers to the newly discovered peers.
         */
        void addDiscoveredPeers(const std::vector<std::shared_ptr<Peer>> &discoveredPeers);

        /**
         * @brief Removes peers that have been inactive for sepcified time duration.
         *
         * @param timeout The duration of inactivity (in seconds) after which a peer is considered inactive.
         */
        void removeInactivePeers(std::chrono::seconds timeout);

        /**
         * @brief Accepts a new peer discovered by the discovery module.
         *
         * @param peer A shared pointer to the newly discovered peer.
         */
        void onPeerDiscovery(std::shared_ptr<Peer> &peer);

        /**
         * @brief Lists all peers currently managed.
         *
         * @return A vector of shared pointers to all managed peers.
         */
        std::vector<std::shared_ptr<Peer>> listPeers() const;

        /**
         * @brief Broadcasts a message to all available peers.
         */
        void broadcast(const std::string& message);
    private:
        /**
         * @brief A map that stores peers by their unique IDs.
         */
        std::unordered_map<std::string, std::shared_ptr<Peer>> peers_;

        /**
         * @brief Mutex to ensure thread-safe access to the peers map.
         */
        mutable std::mutex mutex_;
    };

} // namespace relay

#endif // PEER_MANAGER_H
