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

namespace relay
{

    enum class DiscoveryMessageType : uint8_t
    {
        DISCOVERY_REQUEST = 0,
        DISCOVERY_RESPONSE = 1
    };

    std::string toString(DiscoveryMessageType type);
    size_t messageSize(DiscoveryMessageType type);

    /**
     * @class PeerDiscovery
     * @brief Handles peer discovery using UDP multicast.
     *
     * Enables discovery of peers within the same network via multicast, storing discovered peers
     * as IP:port strings for integration with PeerManager.
     */
    class PeerDiscovery
    {
    public:
        /**
         * @brief Constructs a PeerDiscovery instance.
         * @param multicastIp Multicast group address (e.g., "224.0.0.251").
         * @param multicastPort UDP port for discovery (e.g., 5353).
         * @param localIp Local interface IP to bind to (e.g., "0.0.0.0").
         */
        PeerDiscovery(const std::string &multicastIp, int multicastPort, const std::string &localIp = "0.0.0.0");

        /**
         * @brief Destructor. Stops discovery and cleans up.
         */
        ~PeerDiscovery();

        /**
         * @brief Starts the peer discovery service with sender and listener threads.
         */
        void start();

        /**
         * @brief Stops the peer discovery service.
         */
        void stop();

        /**
         * @brief Sets a custom error handler for discovery errors.
         * @param handler Function taking an error message.
         */
        void setErrorHandler(const std::function<void(const std::string &)> &handler);

        /**
         * @brief Gets the list of discovered peers (IP:port strings).
         * @return Vector of peer addresses.
         */
        std::vector<std::string> getDiscoveredPeers() const;
        
    private:
        std::string multicastIp_;                      ///< Multicast group address.
        int multicastPort_;                            ///< Multicast port.
        std::string localIp_;                          ///< Local interface IP.
        std::shared_ptr<SocketWrapper> socketWrapper_; ///< UDP socket for multicast.
        std::vector<std::string> peers_;               ///< Discovered peers (IP:port).
        mutable std::mutex peersMutex_;                ///< Mutex for peers list.
        std::atomic<bool> stopDiscovery_;              ///< Flag to stop discovery.
        std::unique_ptr<std::thread> senderThread_;    ///< Thread for sending discovery requests.
        std::unique_ptr<std::thread> listenerThread_;  ///< Thread for receiving responses.
        mutable std::mutex mutex_;                     ///< Mutex for thread control.

        /**
         * @brief Sends periodic multicast discovery requests.
         */
        void discoverySender();

        /**
         * @brief Listens for multicast discovery responses and requests.
         */
        void discoveryListener();

        /**
         * @brief Responds to a discovery request from another peer.
         * @param senderAddr Address of the requesting peer.
         */
        void respondToDiscovery(struct ::sockaddr_in &senderAddr);

        /**
         * @brief Handles a discovery response, adding the peer if new.
         * @param response Response message.
         * @param senderAddr Address of the responding peer.
         */
        void handleDiscoveryResponse(const std::string &response, struct ::sockaddr_in &senderAddr);

        /**
         * @brief Logs an error message via the error handler or logger.
         * @param message Error message.
         */
        void logError(const std::string &message) const;
    };

} // namespace relay

#endif