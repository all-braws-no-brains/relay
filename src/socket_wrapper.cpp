#include "relay/socket_wrapper.h"
#include "relay/logger.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

namespace relay
{

    SocketWrapper::SocketWrapper(SocketMode mode) : socketFd_(-1), mode_(mode), isSocketOpen_(false), useIPv6_(false)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int domain = useIPv6_ ? AF_INET6 : AF_INET;
        int type = (mode == SocketMode::UDP) ? SOCK_DGRAM : SOCK_STREAM;

        socketFd_ = socket(domain, type, 0);
        if (socketFd_ == -1)
        {
            const std::string errorMsg = "Failed to create socket: " + std::string(strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
        isSocketOpen_ = true;
        Logger::getInstance().log(LogLevel::INFO, "SocketWrapper initialized. Mode: " + std::string(mode == SocketMode::UDP ? "UDP" : (mode == SocketMode::TCP_SERVER ? "TCP_SERVER" : "TCP_CLIENT")));
    }

    SocketWrapper::SocketWrapper(int socketFd) : socketFd_(socketFd), mode_(SocketMode::TCP_CLIENT), isSocketOpen_(true), useIPv6_(false) {}

    SocketWrapper::~SocketWrapper()
    {
        close();
        Logger::getInstance().log(LogLevel::INFO, "SocketWrapper destroyed.");
    }

    void SocketWrapper::initialize(const std::string &ip, int port, bool useIPv6)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isSocketOpen_)
            throw std::runtime_error("Socket is not open.");
        useIPv6_ = useIPv6;

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0)
        {
            const std::string errorMsg = "Invalid IP address: " + ip;
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::invalid_argument(errorMsg);
        }

        if (mode_ == SocketMode::TCP_SERVER || mode_ == SocketMode::UDP)
        {
            if (bind(socketFd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1)
            {
                const std::string errorMsg = "Failed to bind socket: " + std::string(strerror(errno));
                Logger::getInstance().log(LogLevel::ERROR, errorMsg);
                throw std::runtime_error(errorMsg);
            }
        }
        else if (mode_ == SocketMode::TCP_CLIENT)
        {
            if (connect(socketFd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1)
            {
                const std::string errorMsg = "Failed to connect to server: " + std::string(strerror(errno));
                Logger::getInstance().log(LogLevel::ERROR, errorMsg);
                throw std::runtime_error(errorMsg);
            }
        }
        Logger::getInstance().log(LogLevel::INFO, "Socket initialized at " + ip + ":" + std::to_string(port));
    }

    void SocketWrapper::enableMulticast(const std::string &multicastIp, int multicastPort)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (mode_ != SocketMode::UDP)
            throw std::logic_error("Multicast only supported in UDP mode.");

        struct ip_mreq mreq{};
        inet_pton(AF_INET, multicastIp.c_str(), &mreq.imr_multiaddr);
        mreq.imr_interface.s_addr = INADDR_ANY;
        if (setsockopt(socketFd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
        {
            const std::string errorMsg = "Failed to join multicast group: " + std::string(strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
        Logger::getInstance().log(LogLevel::INFO, "Joined multicast group " + multicastIp + ":" + std::to_string(multicastPort));
    }

    void SocketWrapper::listen(int maxConnections)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (mode_ != SocketMode::TCP_SERVER)
            throw std::logic_error("listen() is only for TCP_SERVER mode.");
        if (::listen(socketFd_, maxConnections) == -1)
        {
            const std::string errorMsg = "Failed to listen on socket: " + std::string(strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
        Logger::getInstance().log(LogLevel::INFO, "Socket listening for connections.");
    }

    std::shared_ptr<SocketWrapper> SocketWrapper::accept()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (mode_ != SocketMode::TCP_SERVER)
            throw std::logic_error("accept() is only for TCP_SERVER mode.");

        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = ::accept(socketFd_, reinterpret_cast<sockaddr *>(&clientAddr), &addrLen);
        if (clientFd == -1)
        {
            const std::string errorMsg = "Failed to accept connection: " + std::string(strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
        Logger::getInstance().log(LogLevel::INFO, "Accepted new connection.");
        return std::make_shared<SocketWrapper>(clientFd);
    }

    size_t SocketWrapper::send(const std::string &data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isSocketOpen_)
            return 0;

        ssize_t bytesSent = ::send(socketFd_, data.c_str(), data.size(), 0);
        if (bytesSent == -1)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to send data: " + std::string(strerror(errno)));
            return 0;
        }
        Logger::getInstance().log(LogLevel::INFO, "Sent " + std::to_string(bytesSent) + " bytes.");
        return static_cast<size_t>(bytesSent);
    }

    size_t SocketWrapper::sendTo(const std::string &data, const struct sockaddr_in &destAddr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isSocketOpen_ || mode_ != SocketMode::UDP)
            return 0;

        ssize_t bytesSent = ::sendto(socketFd_, data.c_str(), data.size(), 0, reinterpret_cast<const sockaddr *>(&destAddr), sizeof(destAddr));
        if (bytesSent == -1)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to sendTo data: " + std::string(strerror(errno)));
            return 0;
        }
        Logger::getInstance().log(LogLevel::INFO, "Sent " + std::to_string(bytesSent) + " bytes to " + inet_ntoa(destAddr.sin_addr) + ":" + std::to_string(ntohs(destAddr.sin_port)));
        return static_cast<size_t>(bytesSent);
    }

    std::string SocketWrapper::receive(size_t bufferSize)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isSocketOpen_)
            return "";

        std::vector<char> buffer(bufferSize);
        ssize_t bytesRead = ::recv(socketFd_, buffer.data(), bufferSize, 0);
        if (bytesRead == -1)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to receive data: " + std::string(strerror(errno)));
            return "";
        }
        else if (bytesRead == 0)
        {
            Logger::getInstance().log(LogLevel::WARNING, "Connection closed by peer.");
            close();
            return "";
        }
        Logger::getInstance().log(LogLevel::INFO, "Received " + std::to_string(bytesRead) + " bytes.");
        return std::string(buffer.data(), bytesRead);
    }

    std::string SocketWrapper::receiveFrom(size_t bufferSize, struct sockaddr_in &senderAddr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isSocketOpen_ || mode_ != SocketMode::UDP)
            return "";

        std::vector<char> buffer(bufferSize);
        socklen_t addrLen = sizeof(senderAddr);
        ssize_t bytesRead = ::recvfrom(socketFd_, buffer.data(), bufferSize, 0, reinterpret_cast<sockaddr *>(&senderAddr), &addrLen);
        if (bytesRead == -1)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to receiveFrom data: " + std::string(strerror(errno)));
            return "";
        }
        Logger::getInstance().log(LogLevel::INFO, "Received " + std::to_string(bytesRead) + " bytes from " + inet_ntoa(senderAddr.sin_addr));
        return std::string(buffer.data(), bytesRead);
    }

    void SocketWrapper::close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isSocketOpen_)
        {
            ::close(socketFd_);
            socketFd_ = -1;
            isSocketOpen_ = false;
            Logger::getInstance().log(LogLevel::INFO, "Socket closed.");
        }
    }

    bool SocketWrapper::isOpen() const
    {
        return isSocketOpen_;
    }

    void SocketWrapper::setErrorHandler(const std::function<void(const std::string &)> &handler)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        errorHandler_ = handler;
    }

    void SocketWrapper::setTimeout(int timeout)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        timeval tv{timeout, 0};
        if (setsockopt(socketFd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
            setsockopt(socketFd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1)
        {
            const std::string errorMsg = "Failed to set socket timeout: " + std::string(strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
    }

    void SocketWrapper::setNonBlocking(bool isNonBlocking)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int flags = fcntl(socketFd_, F_GETFL, 0);
        if (flags == -1 || fcntl(socketFd_, F_SETFL, isNonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == -1)
        {
            const std::string errorMsg = "Failed to set non-blocking mode: " + std::string(strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
    }

    void SocketWrapper::shutdown(bool read, bool write)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int how = (read && write) ? SHUT_RDWR : (read ? SHUT_RD : SHUT_WR);
        if (::shutdown(socketFd_, how) == -1)
        {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to shutdown socket: " + std::string(strerror(errno)));
        }
    }

} // namespace relay