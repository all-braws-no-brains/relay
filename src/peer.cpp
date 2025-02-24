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
    Peer::Peer(const std::string &id, const std::string &ip, int port, std::shared_ptr<SocketWrapper> socket)
        : id_(id), ip_(ip), port_(port), lastActive_(std::chrono::system_clock::now()), socket_(socket) {}

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
     * @brief Sets the metadata associated with the peer.
     */
    void Peer::setMetadata(const std::optional<std::string> &metadata)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        metadata_ = metadata;
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

    bool Peer::sendMessage(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!socket_ || !socket_->isOpen())
        {
            Logger::getInstance().log(LogLevel::WARNING, "Cannot send message, socket closed for Peer: " + id_);
            return false;
        }

        try
        {
            socket_->send(message);
            Logger::getInstance().log(LogLevel::INFO, "Sent message to peer " + id_ + ": " + message);
            return true;
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to send message to peer: " + id_ + ": " + e.what());
        }
    }

    std::string Peer::receiveMessage()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!socket_ || !socket_->isOpen())
        {
            Logger::getInstance().log(LogLevel::WARNING, "Cannot receive message, socket closed for peer: " + id_);
            return "";
        }

        try
        {
            std::string message = socket_->receive(1024);
            Logger::getInstance().log(LogLevel::INFO, "Received message from peer " + id_ + ": " + message);
            return message;
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to receive message from peer " + id_ + ": " + e.what());
            return "";
        }
    }

    bool Peer::isConnected() const
    {
        return socket_ && socket_->isOpen();
    }

    void Peer::closeConnection()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (socket_ && socket_->isOpen())
        {
            socket_->close();
            Logger::getInstance().log(LogLevel::INFO, "Connection closed for peer: " + id_);
        }
    }
} // namespace relays