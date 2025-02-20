#include "relay/peer.h"
#include "relay/logger.h"
#include <iostream>
#include <mutex>

namespace relay
{
    /**
     * @brief Constructs a Peer object with the given ID, IP address, and port.
     *
     * @param id Unique identifier for the peer.
     * @param ip IP address of the peer.
     * @param port Port number of the peer.
     */
    Peer::Peer(const std::string &id, const std::string &ip, int port)
        : id_(id), ip_(ip), port_(port), lastActive_(std::chrono::system_clock::now()) {}

    /**
     * @brief Gets the unique ID of the peer.
     * @return The peer's unique ID.
     */

    std::string Peer::getId() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return id_;
    }

    /**
     * @brief Gets the IP address of the peer.
     * @return The peer's IP address.
     */
    std::string Peer::getIp() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ip_;
    }

    /**
     * @brief Gets the port number of the peer.
     * @return The peer's port number.
     */
    int Peer::getPort() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return port_;
    }

    /**
     * @brief Updates the last active time of the peer.
     */
    void Peer::updateLastActive()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastActive_ = std::chrono::system_clock::now();
    }

    /**
     * @brief Gets the last active time of the peer.
     * @return The last active time of the peer as a timestamp.
     */
    std::chrono::system_clock::time_point Peer::getLastActive() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastActive_;
    }

    /**
     * @brief Gets the metadata associated with the peer.
     * @return The metadata if present, otherwise an empty optional.
     */
    std::optional<std::string> Peer::getMetadata() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return metadata_;
    }

    void Peer::receiveMessage(const std::string& sourceId, const std::string& message) {
        if (sourceId.empty() || message.empty()) {
            Logger::getInstance().log(LogLevel::ERROR, "Received an invalid message or source ID.");
            throw std::invalid_argument("Source ID or message cannot be empty.");
        }

        {
            std::lock_guard<std::mutex> lock(messageQueueMutex_);
            messageQueue_.emplace("From " + sourceId + ": " + message);
        }

        Logger::getInstance().log(LogLevel::INFO, "Message received from peer with ID: "+ sourceId);
    }
    
} // namespace relays