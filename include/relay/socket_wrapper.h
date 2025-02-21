#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <stdexcept>
#include <vector>
#include <optional>
#include <functional>
#include <atomic>

/**
 * @file socket_wrapper.h
 * @brief Defines a custom socket wrapper tailored for thread-safe and modular use.
 */

namespace relay {

/**
 * @enum SocketMode
 * @brief Enum for socket operation mode.
 */
enum class SocketMode {
    SERVER, 
    CLIENT
};

/**
 * @class SocketWrapper
 * @brief A thread-safe custom socket wrapper for server and client operations.
 * 
 * The SocketWrapper provides an abstraction over standard sockets,
 * offering  a thread-safe , easy-to-use interface for data transmission.
 */
class SocketWrapper {
public:
    /**
     * @brief Constructor for SocketWrapper.
     * @param mode The mode of the socket (SERVER or CLIENT).
     */
    explicit SocketWrapper(SocketMode mode);

    /**
     * @brief Destructor for SocketWrapper.
     */
    ~SocketWrapper();

    /**
     * @brief Specialized constructor for accepted sockets.
     */
    SocketWrapper(int socketFd);

    /**
     * @brief Initializes the socket (bind for server, connects for client).
     * @param ip The IP address to bind/connect to.
     * @param port The port to ind/connect to.
     */
    void initialize(const std::string& ip, int port, bool useIPv6 = false);

    /**
     * @brief Strts the server in listening mode.
     * @param maxConnections Maximum number of pending conections for the socket. 
     */
    void listen(int maxConnections);

    /**
     * @brief Accepts a client connection (only for server mode).
     * @return A shared pointer to the new SocketWrapper for the client connction.
     */
    std::shared_ptr<SocketWrapper> accept();

    /**
     * @brief Sends data through the socket.
     * @param data The data to send.
     * @return The number of bytes sent.
     */
    size_t send(const std::string& data);

    /**
     * @brief Receives data from the socket.
     * @param bufferSize The size of the buffer to receive data.
     * @return The received data as a string.
     */
    std::string receive(size_t bufferSize);

    /**
     * @brief Closes the socket connection.
     */
    void close();

    /**
     * @brief Checks if the socket is open.
     * @return True is the socket is open, false otherwise.
     */
    bool isOpen() const;

    /**
     * @brief Sets a custom error handler for socket operations.
     * @param handler A function to handle errors (takes an error message as parameter).
     */
    void setErrorHandler(const std::function<void(const std::string&)>& handler);

    /**
     * @brief Sets the timout for socket operations.
     * @param timeout The timeout duration in seconds.
     */
    void setTimeout(int timeout);

    /**
     * @brief Toggles non-blocking mode for the socket.
     * @param isNonBlocking Set to true to enable non-blocking mode, false to diable it.
     */
    void setNotBlocking(bool isNonBlocking);

    /**
     * @brief Shuts down the socket (optionally read, write, or both)
     * @param read If true, shuts down the read operations.
     * @param write If true, shuts down the write operations.
     */
    void shutdown(bool read = true, bool write = true);

private:
    int socketFd_; ///< File descriptor for the socket.
    SocketMode mode_;
    mutable std::mutex mutex_;
    std::optional<std::function<void(const std::string&)>> errorHandler_;
    std::atomic<bool> isSocketOpen_;
    bool useIPv6_;

    /**
     * @brief Disables copy semantics for thread safety.
     */
    SocketWrapper(const SocketWrapper&) = delete;
    SocketWrapper& operator=(const SocketWrapper&) = delete;

    /**
     * @brief Enables move semantics.
     */
    SocketWrapper(SocketWrapper& other) noexcept;
    SocketWrapper& operator=(SocketWrapper&& other) noexcept;

    /**
     * @brief Internal helper to handle errors.
     * @param errorMessage The error message to handle.
     */
    void handleError(const std::string& errorMessage);

    /**
     * @brief Ensures proper cleanup of resources.
     */
    void cleanup();
};
}
#endif  