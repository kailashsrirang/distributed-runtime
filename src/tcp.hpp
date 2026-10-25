#pragma once

#include "unique_fd.hpp"

#include <cstdint>

// Functions returning UniqueFd transfer ownership to the caller
// Functions accepting int fd only borrow access. must not close fd
UniqueFd listen_local(std::uint16_t port);
UniqueFd accept_worker(int listener_fd);
UniqueFd connect_local(std::uint16_t port);

void send_byte(int fd, std::uint8_t value);
std::uint8_t receive_byte(int fd);