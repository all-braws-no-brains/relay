#include "../include/relay/peer_discovery.h"
#include "../include/relay/logger.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace relay
{

    std::string toString(DiscoveryMessageType type)
    {
        switch (type)
        {
        case DiscoveryMessageType::DISCOVERY_REQUEST:
            return "DISCOVERY_REQUEST";
        case DiscoveryMessageType::DISCOVERY_RESPONSE:
            return "DISCOVERY_RESPONSE";
        default:
            throw std::invalid_argument("Unknown DiscoveryMessageType");
        }
    }

    size_t messageSize(DiscoveryMessageType type)
    {
        return toString(type).size();
    }

    PeerDiscovery::PeerDiscovery(const std::string &multicastIp, int multicastPort, const std::string &localIp)
        : multicastIp_(multicastIp),
          multicastPort_(multicastPort),
          localIp_(localIp),
          stopDiscovery_(false),
          socketWrapper_(std::make_shared<SocketWrapper>(SocketMode::UDP))
    {
        socketWrapper_->initialize(localIp_, multicastPort_);          // Bind to local interface
        socketWrapper_->enableMulticast(multicastIp_, multicastPort_); // Join multicast group
    }

    PeerDiscovery::~PeerDiscovery()
    {
        stop();
    }

    void PeerDiscovery::start()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if ((senderThread_ && senderThread_->joinable()) || (listenerThread_ && listenerThread_->joinable()))
        {
            Logger::getInstance().log(LogLevel::WARNING, "Peer discovery is already running.");
            return;
        }

        stopDiscovery_ = false;

        senderThread_ = std::make_unique<std::thread>(&PeerDiscovery::discoverySender, this);
        listenerThread_ = std::make_unique<std::thread>(&PeerDiscovery::discoveryListener, this);

        Logger::getInstance().log(LogLevel::INFO, "Started peer discovery on " + multicastIp_ + ":" + std::to_string(multicastPort_));
    }

    void PeerDiscovery::stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopDiscovery_ = true;
        }

        if (senderThread_ && senderThread_->joinable())
        {
            senderThread_->join();
        }
        if (listenerThread_ && listenerThread_->joinable())
        {
            listenerThread_->join();
        }

        socketWrapper_->close();
        Logger::getInstance().log(LogLevel::INFO, "Peer discovery stopped.");
    }

    void PeerDiscovery::setErrorHandler(const std::function<void(const std::string &)> &handler)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        socketWrapper_->setErrorHandler(handler); // Delegate to socket
    }

    std::vector<std::string> PeerDiscovery::getDiscoveredPeers() const
    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        return peers_;
    }

    void PeerDiscovery::discoverySender()
    {
        while (!stopDiscovery_.load())
        {
            try
            {
                std::string discoveryMessage = toString(DiscoveryMessageType::DISCOVERY_REQUEST);
                struct ::sockaddr_in destAddr{};
                destAddr.sin_family = AF_INET;
                destAddr.sin_port = htons(multicastPort_);
                inet_pton(AF_INET, multicastIp_.c_str(), &destAddr.sin_addr);

                size_t bytesSent = socketWrapper_->sendTo(discoveryMessage, destAddr);
                Logger::getInstance().log(LogLevel::INFO, "Broadcasted discovery packet: " + discoveryMessage + " (" + std::to_string(bytesSent) + " bytes)");
            }
            catch (const std::exception &e)
            {
                logError("Failed to broadcast discovery packet: " + std::string(e.what()));
            }
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Avoid flooding
        }
    }

    void PeerDiscovery::discoveryListener()
    {
        while (!stopDiscovery_.load())
        {
            try
            {
                struct ::sockaddr_in senderAddr{};
                std::string response = socketWrapper_->receiveFrom(1024, senderAddr);
                if (!response.empty())
                {
                    if (response == toString(DiscoveryMessageType::DISCOVERY_REQUEST))
                    {
                        respondToDiscovery(senderAddr);
                    }
                    else if (response == toString(DiscoveryMessageType::DISCOVERY_RESPONSE))
                    {
                        handleDiscoveryResponse(response, senderAddr);
                    }
                }
            }
            catch (const std::exception &e)
            {
                logError("Error receiving discovery response: " + std::string(e.what()));
            }
        }
    }

    void PeerDiscovery::respondToDiscovery(struct ::sockaddr_in &senderAddr)
    {
        try
        {
            std::string response = toString(DiscoveryMessageType::DISCOVERY_RESPONSE);
            size_t bytesSent = socketWrapper_->sendTo(response, senderAddr);
            Logger::getInstance().log(LogLevel::DEBUG, "Sent discovery response (" + std::to_string(bytesSent) + " bytes) to " + inet_ntoa(senderAddr.sin_addr));
        }
        catch (const std::exception &e)
        {
            logError("Failed to send discovery response: " + std::string(e.what()));
        }
    }

    void PeerDiscovery::handleDiscoveryResponse(const std::string &response, struct ::sockaddr_in &senderAddr)
    {
        std::string peerAddr = std::string(inet_ntoa(senderAddr.sin_addr)) + ":" + std::to_string(ntohs(senderAddr.sin_port));
        Logger::getInstance().log(LogLevel::DEBUG, "Received discovery response: " + response + " from " + peerAddr);

        std::lock_guard<std::mutex> lock(peersMutex_);
        bool exists = false;
        for (const std::string& peer : peers_) {
            if (peer == peerAddr) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            peers_.push_back(peerAddr);
            Logger::getInstance().log(LogLevel::INFO, "Added new peer: " + peerAddr);
        }
    }

    void PeerDiscovery::logError(const std::string &message) const
    {
        Logger::getInstance().log(LogLevel::ERROR, message);
    }

} // namespace relay