#include "relay/peer_manager.h"
#include "relay/logger.h"
#include <stdexcept>
#include <algorithm>


namespace relay {

PeerManager::PeerManager() {};

PeerManager::~PeerManager() {};


void PeerManager::addPeer(const std::shared_ptr<Peer>& peer) {
    if (!peer) {
        Logger::getInstance().log(LogLevel::ERROR, "Cannot add a null peer.");
        throw std::invalid_argument("Cannot add a null peer.");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto result = peers_.emplace(peer->getId(), peer);
    if (!result.second) {
        Logger::getInstance().log(LogLevel::ERROR, "Peer with ID alrady exists: " + peer->getId());
        throw std::runtime_error("Peer with ID " + peer->getId() + " already exists.");
    }

    Logger::getInstance().log(LogLevel::INFO, "Added peer with ID: " + peer->getId());
}

bool PeerManager::removePeer(const std::string& peerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = peers_.find(peerId);
    if (it != peers_.end()) {
        peers_.erase(it);
        Logger::getInstance().log(LogLevel::INFO, "Removed peer with ID: " + peerId);
        return true;
    }

    Logger::getInstance().log(LogLevel::WARNING, "Attempted to remove non-existing peer with ID: "+ peerId);
    return false;
}

std::shared_ptr<Peer> PeerManager::getPeer(const std::string& peerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = peers_.find(peerId);
    if (it != peers_.end()) {
        return it->second;
    }

    Logger::getInstance().log(LogLevel::WARNING, "Peer with ID not found: " + peerId);
    return nullptr;
}

bool PeerManager::relayMessage(const std::string& sourceId, const std::string& targetId, const std::string& message) {
    std::shared_ptr<Peer> sourcePeer;
    std::shared_ptr<Peer> targetPeer;

    {
        // Scoped lock for thread safety
        std::lock_guard<std::mutex> lock(mutex_);
        sourcePeer = getPeer(sourceId);
        targetPeer = getPeer(targetId);
    }


    if (!sourcePeer) {
        Logger::getInstance().log(LogLevel::ERROR, "Message relay failed: Source peer with ID " + sourceId + " not found.");
        return false;
    }

    if (!targetPeer) {
        Logger::getInstance().log(LogLevel::ERROR, "Message relay failed: Target peer with ID " + targetId + " not found.");
        return false;
    }

    try {
        std::string transformedMessage = "[Relayed] " + message;
        targetPeer->receiveMessage(sourceId, message);
        Logger::getInstance().log(LogLevel::INFO, "Relayed message from "+ sourceId + "to " + targetId);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Message relay failed with exception: " + std::string(e.what()));
        return false;
    }
}

};