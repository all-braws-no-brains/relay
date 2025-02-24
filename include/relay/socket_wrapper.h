#ifndef RELAY_SOCKET_WRAPPER_H
#define RELAY_SOCKET_WRAPPER_H

#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <stdexcept>
#include <vector>
#include <optional>
#include <functional>
#include <atomic>
#include <netinet/in.h>
#include <arpa/inet.h> 

namespace relay
{

    enum class SocketMode
    {
        TCP_SERVER,
        TCP_CLIENT,
        UDP
    };

    /**
     * @class SocketWrapper
     * @brief Thread-safe socket abstraction for TCP and UDP operations.
     */
    class SocketWrapper
    {
    public:
        /**
         * @brief Constructs a SocketWrapper with a specified mode.
         * @param mode TCP_SERVER, TCP_CLIENT, or UDP.
         */
        explicit SocketWrapper(SocketMode mode);

        /**
         * @brief Constructs from an accepted TCP socket FD.
         * @param socketFd File descriptor of an accepted socket.
         */
        SocketWrapper(int socketFd);

        /**
         * @brief Destructor. Closes the socket.
         */
        ~SocketWrapper();

        /**
         * @brief Initializes the socket (bind for servers/UDP, connect for TCP clients).
         * @param ip IP address to bind/connect to.
         * @param port Port to bind/connect to.
         * @param useIPv6 Use IPv6 instead of IPv4 (default: false).
         */
        bool initialize(const std::string &ip, int port, bool useIPv6 = false);

        /**
         * @brief Enables multicast on a UDP socket.
         * @param multicastIp Multicast group address (e.g., "224.0.0.251").
         * @param multicastPort Multicast port.
         */
        void enableMulticast(const std::string &multicastIp, int multicastPort);

        /**
         * @brief Starts listening (TCP server only).
         * @param maxConnections Max pending connections.
         */
        void listen(int maxConnections);

        /**
         * @brief Accepts a TCP client connection (TCP server only).
         * @return Shared pointer to a new SocketWrapper for the client.
         */
        std::shared_ptr<SocketWrapper> accept();

        /**
         * @brief Sends data through the socket.
         * @param data Data to send.
         * @return Bytes sent, or 0 on failure.
         */
        size_t send(const std::string &data);

        /**
         * @brief Sends data to a specific address (UDP only).
         * @param data Data to send.
         * @param destAddr Destination address.
         * @return Bytes sent, or 0 on failure.
         */
        size_t sendTo(const std::string &data, struct ::sockaddr_in &destAddr);

        /**
         * @brief Receives data from the socket.
         * @param bufferSize Buffer size for receiving.
         * @return Received data, or empty string if failed.
         */
        std::string receive(size_t bufferSize);

        /**
         * @brief Receives data with sender address (UDP only).
         * @param bufferSize Buffer size for receiving.
         * @param senderAddr Output parameter for sender’s address.
         * @return Received data, or empty string if failed.
         */
        std::string receiveFrom(size_t bufferSize, struct ::sockaddr_in &senderAddr);

        /**
         * @brief Closes the socket.
         */
        void close();

        /**
         * @brief Checks if the socket is open.
         * @return True if open, false otherwise.
         */
        bool isOpen() const;

        /**
         * @brief Sets an error handler for socket operations.
         * @param handler Function taking an error message.
         */
        void setErrorHandler(const std::function<void(const std::string &)> &handler);

        /**
         * @brief Sets timeout for socket operations.
         * @param timeout Timeout in seconds.
         */
        void setTimeout(int timeout);

        /**
         * @brief Toggles non-blocking mode.
         * @param isNonBlocking True to enable non-blocking.
         */
        void setNonBlocking(bool isNonBlocking);

        /**
         * @brief Shuts down socket read/write operations.
         * @param read Shut down reading.
         * @param write Shut down writing.
         */
        void shutdown(bool read = true, bool write = true);

        int getSocketFd() const { return socketFd_; }
    private:
        int socketFd_; ///< Socket file descriptor.
        SocketMode mode_;
        mutable std::mutex mutex_;
        std::optional<std::function<void(const std::string &)>> errorHandler_;
        std::atomic<bool> isSocketOpen_;
        bool useIPv6_;

        SocketWrapper(const SocketWrapper &) = delete;
        SocketWrapper &operator=(const SocketWrapper &) = delete;
        SocketWrapper(SocketWrapper &&other) noexcept;
        SocketWrapper &operator=(SocketWrapper &&other) noexcept;

        void handleError(const std::string &errorMessage);
        void cleanup();
    };

} // namespace relay

#endif