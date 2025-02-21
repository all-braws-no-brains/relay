#include "relay/socket_wrapper.h"
#include "relay/logger.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> 

namespace relay {

SocketWrapper::SocketWrapper(SocketMode mode) : socketFd_(-1), mode_(mode), isSocketOpen_(false), useIPv6_(false) {
    std::lock_guard<std::mutex> lock(mutex_);

    //Create the socket
    socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ == -1){
        const std::string errorMsg = "Failed to create socket: " + std::string(std::strerror(errno));
        Logger::getInstance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }

    isSocketOpen_ = true;
    Logger::getInstance().log(LogLevel::INFO, "SocketWrapper initialized. Mode: " +
        std::string((mode == SocketMode::SERVER) ? "SERVER" : "CLIENT"));
}  

SocketWrapper::SocketWrapper(int socketFd) 
    : socketFd_(socketFd), mode_(SocketMode::CLIENT), isSocketOpen_(true) {}

SocketWrapper::~SocketWrapper() {
    close();
    Logger::getInstance().log(LogLevel::INFO, "SocketWrapper destroyed.");
}

void SocketWrapper::initialize(const std::string& ip, int port, bool useIPv6) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isSocketOpen_) {
        throw std::runtime_error("Socket is not open.");
    }

    useIPv6_ = useIPv6;
    
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
        const std::string errorMsg = "Invalid IP address: " + ip;
        Logger::getInstance().log(LogLevel::ERROR, errorMsg);
        throw std::invalid_argument(errorMsg);
    }

    if (mode_ == SocketMode::SERVER) {
        if (bind(socketFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
            const std::string errorMsg = "Failed to bind socket: " + std::string(std::strerror(errno));
                Logger::getInstance().log(LogLevel::ERROR, errorMsg);
                throw std::runtime_error(errorMsg);
        }
    } 
    else if (mode_ == SocketMode::CLIENT) {
        if (connect(socketFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
            const std::string errorMsg = "Failed to connect to server: " + std::string(std::strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
    }
    Logger::getInstance().log(LogLevel::INFO, "Connected to server at " + ip + ":" + std::to_string(port));

}

// Start listening (server only)
void SocketWrapper::listen(int maxConnections) {
    if (mode_ != SocketMode::SERVER) {
        throw std::logic_error("listen() can only be used in SERVER mode.");
    }

    if (::listen(socketFd_, maxConnections) == -1) {
        const std::string errorMsg = "Failed to listen on socket: " + std::string(std::strerror(errno));
        Logger::getInstance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }

    Logger::getInstance().log(LogLevel::INFO, "Socket is now listening for connections.");
}

std::shared_ptr<SocketWrapper> SocketWrapper::accept() {
    if (mode_ != SocketMode::SERVER) {
        throw std::logic_error("accept() can only be used in SERVER mode.");
    }

    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = ::accept(socketFd_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientFd == -1){
        const std::string errorMsg = "Failed to accept connection: " + std::string(std::strerror(errno));
        Logger::getInstance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }

    Logger::getInstance().log(LogLevel::INFO, "Accept a new connection.");
    auto clientWrapper = std::make_shared<SocketWrapper>(SocketMode::CLIENT);
    clientWrapper->socketFd_ = clientFd;
    clientWrapper->isSocketOpen_ = true;

    return clientWrapper;
}

size_t SocketWrapper::send(const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    ssize_t bytesSent = ::send(socketFd_, data.c_str(), data.size(), 0);
    if (bytesSent == -1) {
        const std::string errorMsg = "Failed to send data: " + std::string(std::strerror(errno));
        Logger::getInstance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }

    Logger::getInstance().log(LogLevel::INFO, "Sent " + std::to_string(bytesSent) + " bytes.");
    return static_cast<size_t>(bytesSent);
}

std::string SocketWrapper::receive(size_t bufferSize) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<char> buffer(bufferSize);
    ssize_t bytesRead = ::recv(socketFd_, buffer.data(), bufferSize, 0);
    if (bytesRead == -1) {
        const std::string errorMsg = "Failed to receive data: " + std::string(std::strerror(errno));
        Logger::getInstance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    } else if (bytesRead == 0) {
        Logger::getInstance().log(LogLevel::WARNING, "Connection closed by peer.");
        close();
        return "";
    }

    Logger::getInstance().log(LogLevel::INFO, "Received " + std::to_string(bytesRead) + " bytes.");
    return std::string(buffer.data(), bytesRead);
}

void SocketWrapper::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (isSocketOpen_) {
        ::close(socketFd_);
        socketFd_ = -1;
        isSocketOpen_ = false;
        Logger::getInstance().log(LogLevel::INFO, "Socket closed.");
    }
}

bool SocketWrapper::isOpen() const {
    return isSocketOpen_;
}

void SocketWrapper::setErrorHandler(const std::function<void(const std::string&)>& handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    errorHandler_ = handler;
}

void SocketWrapper::setTimeout(int timeout) {
    std::lock_guard<std::mutex> lock(mutex_);

    timeval tv{};
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    if (setsockopt(socketFd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
        setsockopt(socketFd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1)  {
            const std::string errorMsg = "Failed to socket timeout: " + std::string(std::strerror(errno));
            Logger::getInstance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
    }
}

void SocketWrapper::setNotBlocking(bool isNonBlocking) {
    std::lock_guard<std::mutex> lock(mutex_);

    int flags = fcntl(socketFd_, F_GETFL, 0);
    if (flags == -1 || fcntl(socketFd_, F_SETFL, isNonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == -1) {
        const std::string errorMsg = "Failed to set non-blocking mode: " + std::string(std::strerror(errno));
        Logger::getInstance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }
}

void SocketWrapper::shutdown(bool read, bool write) {
    std::lock_guard<std::mutex> lock(mutex_);

    int how = (read && write) ? SHUT_RDWR : (read ? SHUT_RD : SHUT_WR);
    ::shutdown(socketFd_, how);
}

}