#include "tcp.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

//
// |  Call       | Purpose |
// |------------ |-----------------------------------------------------------|
// | `socket()`  | Creates a TCP socket                                      |
// | `bind()`    | Assigns the coordinator’s local address and port          |
// | `listen()`  | Makes the socket accept incoming connection requests      |
// | `accept()`  | Waits for a connection and returns a new connected socket |
// | `connect()` | Establishes the worker’s connection to the coordinator    |
// | `send()`    | Submits bytes for transmission                            |
// | `recv()`    | Receives available bytes                                  |
//
namespace
{
    [[noreturn]] void throw_system_error(const char *operation)
    {
        const int error = errno;

        throw std::system_error(
            error,
            std::generic_category(),
            operation);
    }

    // create socket and return socket fd
    // creating a socket builds machinery needed to communicate over a network
    UniqueFd create_socket()
    {
        UniqueFd socket_fd(::socket(AF_INET, SOCK_STREAM, 0));

        if (!socket_fd.valid())
        {
            throw_system_error("socket");
        }

        return socket_fd;
    }

    // creates socket address structure
    sockaddr_in local_address(std::uint16_t port)
    {
        // sockaddr_in: struct used to store IPv4 network addresses (ip + port)
        sockaddr_in address{};

        address.sin_family = AF_INET;   // IPv4
        address.sin_port = htons(port); // convert port into standard network byte order (big-endian). htons: host to network, 16-bit value

        // sets the target IP address
        // INADDR_LOOPBACK: local loopback address (127.0.0.1)
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // htonl: host to network, 32-bit value

        return address;
    }
}

UniqueFd listen_local(std::uint16_t port)
{
    auto listener = create_socket();

    const int reuse_address = 1;

    // set socket options
    // SO_REUSEADDR: Allows a server to bind to a local address immediately after restarting, bypassing the TIME_WAIT state
    //
    if (::setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address)) == -1)
    {
        throw_system_error("setsockopt");
    }

    const auto address = local_address(port); // create socket struct

    // Link socket to port(address)
    if (::bind(listener.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1)
    {
        throw_system_error("bind");
    }

    // prepare socket to accept incoming client connection requests. passive
    if (::listen(listener.get(), 1) == -1)
    {
        throw_system_error("listen");
    }

    return listener;
}

UniqueFd accept_worker(int listener_fd)
{
    while (true)
    {
        // accept. create new socket for the connection. returns new socket fd,
        // accept blocks in no request currently
        const int fd = ::accept(listener_fd, nullptr, nullptr);

        if (fd >= 0)
        {
            return UniqueFd(fd);
        }

        if (errno == EINTR) // singal interrupted operation
        {
            continue;
        }

        throw_system_error("accept");
    }
}

UniqueFd connect_local(std::uint16_t port)
{
    auto connection = create_socket();
    const auto address = local_address(port);

    if (::connect(connection.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1)
    {
        throw_system_error("connect");
    }

    return connection;
}

void send_byte(int fd, std::uint8_t value)
{
    while (true)
    {
        const auto sent = ::send(fd, &value, sizeof(value), MSG_NOSIGNAL);

        if (sent == 1)
        {
            return;
        }

        if (sent == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            throw_system_error("send");
        }

        throw std::runtime_error("send made no progress");
    }
}

std::uint8_t receive_byte(int fd)
{
    std::uint8_t value = 0;

    while (true)
    {
        const auto received = ::recv(fd, &value, sizeof(value), 0);

        if (received == 1)
        {
            return value;
        }

        if (received == 0)
        {
            throw std::runtime_error(
                "Peer closed the connection before sending a byte");
        }

        if (errno == EINTR)
        {
            continue;
        }

        throw_system_error("recv");
    }
}

void send_all(int fd, std::span<const std::uint8_t> bytes)
{
    std::size_t offset = 0;

    while (offset < bytes.size())
    {
        const auto sent = ::send(
            fd,
            bytes.data() + offset,
            bytes.size() - offset,
            MSG_NOSIGNAL);

        if (sent > 0)
        {
            offset += static_cast<std::size_t>(sent);
            continue;
        }

        if (sent == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            throw_system_error("send");
        }

        throw std::runtime_error("send made no progress");
    }
}

bool receive_exact(int fd, std::span<std::uint8_t> bytes)
{
    std::size_t offset = 0;

    while (offset < bytes.size())
    {
        const auto received = ::recv(fd, bytes.data() + offset, bytes.size() - offset, 0);

        if (received > 0)
        {
            offset += static_cast<std::size_t>(received);
            continue;
        }

        if (received == 0)
        {
            if (offset == 0)
            {
                return false;
            }
            throw std::runtime_error("Connection closed during a read");
        }

        if (errno == EINTR)
        {
            continue;
        }

        throw_system_error("recv");
    }

    return true;
}