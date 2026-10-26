#pragma once

#include "unique_fd.hpp"
#include <cstddef>
#include <span>
#include <cstdint>

// Functions returning UniqueFd transfer ownership to the caller
// Functions accepting int fd only borrow access. must not close fd
UniqueFd listen_local(std::uint16_t port);
UniqueFd accept_worker(int listener_fd);
UniqueFd connect_local(std::uint16_t port);

void send_byte(int fd, std::uint8_t value);
std::uint8_t receive_byte(int fd);

void send_all(int fd, std::span<const std::uint8_t> bytes);

// returns false only if EOF occurs before any bytes are received.
// Throws error if EOF occurs after receiving only part of the buffer.
bool receive_exact(int fd, std::span<std::uint8_t> bytes);