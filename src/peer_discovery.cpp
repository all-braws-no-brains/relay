#include "relay/peer_manager.h"
#include "relay/peer_discovery.h"
#include "relay/logger.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace relay {

std::string toString(DiscoveryMessageType type) {
    switch (type) {
    case DiscoveryMessageType::DISCOVERY_REQUEST:
        return "DISCOVERY_REQUEST";
    case DiscoveryMessageType::DISCOVERY_RESPONSE:
        return "DISCOVERY_RESPONSE";
    default:
        throw std::invalid_argument("Unknown DiscoveryMessageType");
    }
}

size_t messageSize(DiscoveryMessageType type) {
    return toString(type).size();
}


PeerDiscovery::PeerDiscovery(const std::string& multicastIp, int multicastPort)
    : multicastIp_(multicastIp),
      multicastPort_(multicastPort),
      stopDiscovery_(false),
      socketWrapper_(std::make_shared<SocketWrapper>(SocketMode::CLIENT)) {}

PeerDiscovery::~PeerDiscovery() {
    stop();
}

void PeerDiscovery::start() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (discoveryThread_ && discoveryThread_->joinable()) {
            Logger::getInstance().log(LogLevel::WARNING, "Peer discovery is already running.");
            return;
        }

        stopDiscovery_ = false;
    }

    // Start the thread for peer discovery
    discoveryThread_ = std::make_unique<std::thread>([this]() {
        try {
            Logger::getInstance().log(LogLevel::INFO, "Starting peer discovery...");

            // Initialize the multicast socket
            socketWrapper_->initialize(multicastIp_, multicastPort_);

            while (!stopDiscovery_.load()) {
                broadcastDiscoveryPacket();
                receiveDiscoveryResponses();

                // Sleep to avoid flooding the network
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error in peer discovery: " + std::string(e.what()));
        }
    });
}

void PeerDiscovery::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopDiscovery_ = true;
    }

    if (discoveryThread_ && discoveryThread_->joinable()) {
        discoveryThread_->join();
    }

    Logger::getInstance().log(LogLevel::INFO, "Peer discovery stopped.");
}

void PeerDiscovery::broadcastDiscoveryPacket() {
    try {
        std::string discoveryMessage = "DISCOVERY_REQUEST";

        // Send the discovery message to the multicast group
        size_t bytesSent = socketWrapper_->send(discoveryMessage);

        Logger::getInstance().log(LogLevel::INFO, "Broadcasted discovery packet: " + discoveryMessage + " (" + std::to_string(bytesSent) + " bytes sent)");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to broadcast discovery packet: " + std::string(e.what()));
    }
}

void PeerDiscovery::receiveDiscoveryResponses() {
   while(!stopDiscovery_.load()) {
        try {
            std::string response = socketWrapper_->receive(1024);
            if (!response.empty()) {
                handleDiscoveryResponse(response);
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error received discovery response: " + std::string(e.what()));
        }
   }
}

void PeerDiscovery::handleDiscoveryResponse(const std::string& response) {
    Logger::getInstance().log(LogLevel::DEBUG, "Handling discovery response: " + response);

    std::lock_guard<std::mutex> lock(peersMutex_);
   
    bool exists = false;
    for (const auto& peer : peers_) {
        if (peer == response) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        peers_.push_back(response);
        Logger::getInstance().log(LogLevel::INFO, "Added a new peer: " + response);
    } else {
        Logger::getInstance().log(LogLevel::INFO, "Peer already exists: " + response);
    }
}

std::vector<std::string> PeerDiscovery::getDiscoveredPeers() const {
    std::lock_guard<std::mutex> lock(peersMutex_);
    return peers_;
}

void PeerDiscovery::logError(const std::string& message) const {
    Logger::getInstance().log(LogLevel::ERROR, message);
}

} // namespace relay
