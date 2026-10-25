#include "unique_fd.hpp"

#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
    void require(bool condition, const char *message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    bool is_closed(int fd)
    {
        errno = 0;
        const int result = ::fcntl(fd, F_GETFD);

        return result == -1 && errno == EBADF;
    }
}

int main()
{
    try
    {
        UniqueFd empty;

        require(!empty.valid(), "Default owner should be empty");
        require(empty.get() == -1, "Empty owner should contain -1");

        // pipes are a one-way communication channel.
        int pipe_fds[2];

        // system call to create a pipe, which returns two file descriptors: one for reading and one for writing
        require(::pipe(pipe_fds) == 0, "pipe failed");

        // initialize UniqueFd objects for the read and write ends of the pipe
        UniqueFd reader(pipe_fds[0]);
        UniqueFd writer(pipe_fds[1]);

        const int read_fd = reader.get();
        const int write_fd = writer.get();

        require(reader.valid(), "Reader should own a descriptor");
        require(writer.valid(), "Writer should own a descriptor");

        UniqueFd moved(std::move(reader));

        require(!reader.valid(), "Move did not empty the source");
        require(moved.get() == read_fd, "Move did not transfer the descriptor");

        writer = std::move(moved);

        require(!moved.valid(), "Move assignment did not empty source");
        require(writer.get() == read_fd, "Move assignment did not transfer descriptor");
        require(is_closed(write_fd), "Move assignment did not close previous descriptor");

        const int released_fd = writer.release();

        require(!writer.valid(), "Release did not empty owner");
        require(released_fd == read_fd, "Release returned wrong descriptor");
        require(::fcntl(released_fd, F_GETFD) != -1, "Release incorrectly closed the descriptor");

        UniqueFd reclaimed(released_fd);

        // return -1 if dup failed
        const int duplicate_fd = ::dup(reclaimed.get());

        require(duplicate_fd >= 0, "dup failed");

        // objects instantiated in a block will be destroyed when the block ends. Therefore, scoped should be destroyed at the end of this block, and its destructor should close the duplicate_fd.
        {
            UniqueFd scoped(duplicate_fd);

            require(scoped.valid(), "Scoped owner should be valid");
        }

        require(is_closed(duplicate_fd), "Destructor did not close descriptor");

        // close fd
        reclaimed.reset();

        require(!reclaimed.valid(), "Reset did not empty owner");
        require(is_closed(read_fd), "Reset did not close descriptor");

        reclaimed.reset();

        require(!reclaimed.valid(), "Repeated reset should remain empty");

        std::cout << "All descriptor ownership tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}