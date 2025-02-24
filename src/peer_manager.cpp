#include "../include/relay/peer_manager.h"
#include "../include/relay/logger.h"
#include <stdexcept>
#include <algorithm>

namespace relay
{

    PeerManager::PeerManager() = default;

    PeerManager::~PeerManager() = default;

    void PeerManager::addPeer(const std::shared_ptr<Peer> &peer)
    {
        if (!peer)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Cannot add a null peer.");
            throw std::invalid_argument("Cannot add a null peer.");
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (peers_.find(peer->getId()) != peers_.end())
            {
                Logger::getInstance().log(LogLevel::ERROR, "Peer with ID already exists: " + peer->getId());
                return;
            }
            peers_.emplace(peer->getId(), peer);
        }

        Logger::getInstance().log(LogLevel::INFO, "Added peer with ID: " + peer->getId());
    }

    bool PeerManager::removePeer(const std::string &peerId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = peers_.find(peerId);
        if (it != peers_.end())
        {
            peers_.erase(it);
            Logger::getInstance().log(LogLevel::INFO, "Removed peer with ID: " + peerId);
            return true;
        }

        Logger::getInstance().log(LogLevel::WARNING, "Attempted to remove non-existing peer with ID: " + peerId);
        return false;
    }

    std::shared_ptr<Peer> PeerManager::getPeer(const std::string &peerId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = peers_.find(peerId);
        if (it != peers_.end())
        {
            return it->second;
        }

        Logger::getInstance().log(LogLevel::WARNING, "Peer with ID not found: " + peerId);
        return nullptr;
    }

    bool PeerManager::relayMessage(const std::string &sourceId, const std::string &targetId, const std::string_view message)
    {
        std::shared_ptr<Peer> sourcePeer = nullptr;
        std::shared_ptr<Peer> targetPeer = nullptr;

        {
            // Scoped lock for thread safety
            std::lock_guard<std::mutex> lock(mutex_);
            sourcePeer = peers_.find(sourceId) != peers_.end() ? peers_.at(sourceId) : nullptr;
            targetPeer = peers_.find(targetId) != peers_.end() ? peers_.at(targetId) : nullptr;
        }

        if (!sourcePeer)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Message relay failed: Source peer with ID " + sourceId + " not found.");
            return false;
        }

        if (!targetPeer)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Message relay failed: Target peer with ID " + targetId + " not found.");
            return false;
        }

        try
        {
            std::string transformedMessage = "[Relayed] " + std::string(message);
            if (targetPeer->sendMessage(transformedMessage))
            {
                sourcePeer->updateLastActive();
                targetPeer->updateLastActive();
                Logger::getInstance().log(LogLevel::INFO, "Relayed message from " + sourceId + " to " + targetId + ": " + transformedMessage);
                return true;
            }
            else
            {
                Logger::getInstance().log(LogLevel::ERROR, "Message relay failed. Could not send message to target: " + targetId);
                return false;
            }
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Message relay failed with exception: " + std::string(e.what()));
            return false;
        }
    }

    void PeerManager::addDiscoveredPeers(const std::vector<std::shared_ptr<Peer>> &discoveredPeers)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &peer : discoveredPeers)
        {
            if (peer && peers_.find(peer->getId()) == peers_.end())
            {
                peers_.emplace(peer->getId(), peer);
                Logger::getInstance().log(LogLevel::INFO, "Discovered and added peer with ID: " + peer->getId());
            }
            else if (peer)
            {
                Logger::getInstance().log(LogLevel::INFO, "Skipped adding already known peer with ID: " + peer->getId());
            }
        }
    }

    void PeerManager::removeInactivePeers(std::chrono::seconds timeout)
    {
        auto now = std::chrono::system_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = peers_.begin(); it != peers_.end();)
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second->getLastActive()) > timeout)
            {
                Logger::getInstance().log(LogLevel::INFO, "Removing inactive peer with ID: " + it->first);
                it = peers_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void PeerManager::onPeerDiscovery(std::shared_ptr<Peer> &peer)
    {
        if (!peer)
            return;

        addPeer(peer);
        Logger::getInstance().log(LogLevel::INFO, "Discovered new peer with ID: " + peer->getId());
    }

    std::vector<std::shared_ptr<Peer>> PeerManager::listPeers() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<Peer>> peerList;
        peerList.reserve(peers_.size());
        for (const auto &[id, peer] : peers_)
        {
            peerList.push_back(peer);
        }
        return peerList;
    }
};