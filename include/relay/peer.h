#ifndef RELAY_PEER_H
#define RELAY_PEER_H

#include <string>
#include <mutex>
#include <chrono>
#include <optional>
#include <queue>
#include "../relay/socket_wrapper.h"

/**
 * @file peer.h
 * @brief Defines the Peer class, representing a single peer in the P2P network.
 */

namespace relay
{

    /**
     * @class Peer
     * @brief Represents an individual peer in the P2P network.
     *
     * The Peer class stores information about a peer, including its unique ID,
     * IP address, port, and optional metadata. It is designed to be thread-safe,
     * allowing concurrent access from multiple threads.
     */
    class Peer
    {
    public:
        /**
         * @brief Constructs a Peer object with the given ID, IP address, and port.
         *
         * @param id Unique identifier for the peer.
         * @param ip IP address of the peer.
         * @param port Port number of the peer.
         * @param socket Shared pointer to the peer's socket connection.
         */
        Peer(const std::string &id, const std::string &ip, int port, std::shared_ptr<SocketWrapper> socket);

        /**
         * @brief Gets the unique ID of the peer.
         * @return The peer's unique ID.
         */
        std::string getId() const;

        /**
         * @brief Gets the IP address of the peer.
         * @return The peer's IP address.
         */
        std::string getIp() const;

        /**
         * @brief Gets the port number of the peer.
         * @return The peer's port number.
         */
        int getPort() const;

        /**
         * @brief Updates the last active time of the peer.
         */
        void updateLastActive();

        /**
         * @brief Gets the last active time of the peer.
         * @return The last active time of the peer as a timestamp.
         */
        std::chrono::system_clock::time_point getLastActive() const;

        /**
         * @brief Sets optional metadata for the peer.
         * @param metadata Key-value string to represent additional peer information.
         */
        void setMetadata(const std::optional<std::string> &metadata);

        /**
         * @brief Gets the metadata associated with the peer.
         * @return The metadata if present, otherwise an empty optional.
         */
        std::optional<std::string> getMetadata() const;

        /**
         * @brief Sends a message to this peer.
         *
         * @param message The message to be sent.
         * @return True if the message was successfully sent, false otherwise.
         */
        bool sendMessage(const std::string &message);

        /**
         * @brief Receives a message from this peer.
         *
         * @return The received message.
         */
        std::string receiveMessage();

        /**
         * @brief Checks if the peer is connected.
         * @return True if the peers is connected, false otherwise.
         */
        bool isConnected() const;

        /**
         * @brief Closes the peer connection.
         */
        void closeConnection();

        /**
         * @brief Accepts multiple clients
         * @param maxClients Maximum number of clients that can be accepted
         */
        void acceptClients(int maxClients);

        /**
         * @brief Returns the socket occupied by the peer
         * @return Returns the socket occupied by the peer
         */
        std::shared_ptr<SocketWrapper> getSocket() const { return socket_; }

        /**
         * @brief Returns the list of clients of the peer
         * @return Returns the list of clients of the peer
         */
        std::vector<std::shared_ptr<SocketWrapper>> getClients() const { return clients_; }

        /**
         * [DEMO]
         * @brief Following methods are for the GUI feature
         */
        int64_t getLatency() const { return latencyMs_; }
        int getMessagesSent() const { return messagesSent_; }
        int getMessagesReceived() const { return messagesReceived_; }
        size_t getBytesSent() const { return bytesSent_; }
        size_t getBytesReceived() const { return bytesReceived_; }
        bool isConnected() const { return isConnected_; }
        void updateLatency()
        {
            if (lastSent_ != std::chrono::steady_clock::time_point() && lastReceived_ != std::chrono::steady_clock::time_point())
            {
                latencyMs_ = std::chrono::duration_cast<std::chrono::milliseconds>(lastReceived_ - lastSent_).count();
            }
        }

    private:
        std::string id_;                                   ///< Unique identifier of the peer.
        std::string ip_;                                   ///< IP address of the peer.
        int port_;                                         ///< Port number of the peer.
        std::chrono::system_clock::time_point lastActive_; ///< Timestamp of the last activity.
        std::optional<std::string> metadata_;              ///< Optional metadata associated with the peer.

        mutable std::mutex mutex_; ///< Mutex for thread-safe access.

        std::shared_ptr<SocketWrapper> socket_; ///< Peer's socket connection.
        std::vector<std::shared_ptr<SocketWrapper>> clients_;
        mutable std::mutex messageQueueMutex_;
        std::queue<std::string> messageQueue_;

        std::chrono::steady_clock::time_point lastSent_;
        std::chrono::steady_clock::time_point lastReceived_;
        int64_t latencyMs_;
        int messagesSent_;
        int messagesReceived_;
        size_t bytesSent_;
        size_t bytesReceived_;
        bool isConnected_;
    };

} // namespace relay

#endif
