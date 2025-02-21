#ifndef RELAY_PEER_DISCOVERY
#define RELAY_PEER_DISCOVERY

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <functional>
#include <memory>
#include "peer_manager.h"
#include "socket_wrapper.h"

namespace relay {

/**
 * @enum DiscoveryMessageType
 * @brief Enum for discovery message types.
 */
enum class DiscoveryMessageType : uint8_t {
    DISCOVERY_REQUEST = 0,
    DISCOVERY_RESPONSE = 1
};

/**
 * @brief Utility function to get the string representation of a discovery message type.
 * @param type The discovery message type.
 * @return The corresponding string representation.
 */
std::string toString(DiscoveryMessageType type);

/**
 * @brief Utility function to get the size of discovery message type as string.
 * @param type The discovery message type.
 * @return The size of the string representation.
 */
size_t messageSize(DiscoveryMessageType type);

/// -------------------------------------------------------------------------------------

/**
 * @class PeerDiscovery
 * @brief Handles peer discovery and automatic addition of peers to the routing table.
 * 
 * This class enables peer discovery within the same network using multicast or broadcast.
 * Discovered peers are stored and can be retrieved for further operations.
 */
class PeerDiscovery {
public:
    /**
     * @brief Constructor for PeerDiscovery.
     * @param multicastIp Multicast group address (e.g., "224.0.0.1") or broadcast address.
     * @param multicastPort UDP port used for discovery.
     */
    PeerDiscovery(const std::string& multicastIp, int multicastPort);

    /**
     * @brief Destructor for PeerDiscovery. Stops discovery threads and cleans up resources.
     */
    ~PeerDiscovery();

    /**
     * @brief Starts the peer discovery service.
     */
    void start();

    /**
     * @brief Stops the peer discovery service.
     */
    void stop();

    /**
     * @brief Sets a custom error handler for discovery logic.
     * @param handler A function to handle errors (takes an error message as a parameter).
     */
    void setErrorHandler(const std::function<void(const std::string&)>& handler);

    /**
     * @brief Retrieves the list of currently discovered peers.
     * @return A vector containing the addresses of discovered peers.
     */
    std::vector<std::string> getDiscoveredPeers() const;

private:
    std::string multicastIp_;                              ///< The multicast IP address.
    int multicastPort_;                                    ///< The multicast port.
    std::shared_ptr<SocketWrapper> socketWrapper_;         ///< Socket wrapper for network operations.
    std::vector<std::string> peers_;                       ///< List of discovered peers.
    mutable std::mutex peersMutex_;                        ///< Mutex for accessing the peers list.

    std::atomic<bool> stopDiscovery_;                      ///< Flag to control the discovery process.
    std::unique_ptr<std::thread> discoveryThread_;         ///< Thread for running the discovery process.
    mutable std::mutex mutex_;                             ///< Mutex for thread-safe discovery start/stop.

    /**
     * @brief Sends periodic discovery requests to other peers on the network.
     */
    void broadcastDiscoveryPacket();

    /**
     * @brief Receives responses from peers after broadcasting a discovery packet.
     */
    void receiveDiscoveryResponses();

    /**
     * @brief Handles a discovery response and adds the peer to the discovered peers list.
     * @param response The response message from the peer.
     */
    void handleDiscoveryResponse(const std::string& response);

    /**
     * @brief Utility function to log errors.
     * @param message The error message.
     */
    void logError(const std::string& message) const;

    /**
     * @brief Reserved for future functionality: Sends discovery requests periodically.
     */
    void discoverySender(); // Reserved for future functionality.

    /**
     * @brief Reserved for future functionality: Listens for discovery requests.
     */
    void discoveryListener(); // Reserved for future functionality.

    /**
     * @brief Reserved for future functionality: Responds to a discovery request.
     * @param senderAddr Address of the requesting peer.
     */
    void respondToDiscovery(struct sockaddr_in senderAddr); // Reserved for future functionality.
};

} // namespace relay

#endif
