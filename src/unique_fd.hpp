#pragma once

#include <unistd.h>

#include <utility>

class UniqueFd
{
public:
    // creates an empty, uninitialized file wrapper
    UniqueFd() noexcept = default;

    // explicit: prevent implicit conversion from int to UniqueFd
    explicit UniqueFd(int fd) noexcept
        : fd_(fd)
    {
    }

    ~UniqueFd() noexcept
    {
        // reset will close the file descriptor if it is valid
        reset();
    }
    // copy constructor take in a type of itself as an argument
    // Copy Constructor (A a2(a1); or A a2 = a1;):
    // do not generate a copy constructor for this class (=delete)
    UniqueFd(const UniqueFd &) = delete;

    // Copy Assignment Operator (a2 = a1;) overwrite contents of old object with contents of new object
    // do not generate
    UniqueFd &operator=(const UniqueFd &) = delete;

    // Move constructor. Callers can use std::move to indicate that they want to transfer ownership of the file descriptor from one UniqueFd object to another.
    UniqueFd(UniqueFd &&other) noexcept
        : fd_(other.release())
    {
    }

    // move assignment operator handles transferring ownership when both objects already exist
    UniqueFd &operator=(UniqueFd &&other) noexcept
    {
        if (this != &other)
        {
            //  clear destination object before taking ownership of the source object's file descriptor
            reset();
            // get fd from other and set it to -1 in other
            fd_ = other.release();
        }

        return *this;
    }

    // nodiscard attribute indicates that the return value of the function should not be ignored by the caller. If the caller ignores the return value, the compiler may generate a warning.
    [[nodiscard]] int get() const noexcept
    {
        return fd_;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return fd_ >= 0;
    }

    [[nodiscard]] int release() noexcept
    {
        // This "releases" ownership of the file descriptor from the UniqueFd object, allowing the caller to take control of it without closing it.
        return std::exchange(fd_, -1);
    }

    void reset() noexcept
    {
        const int old_fd = release();

        // ensure we dont try to close a fd that isn't open
        if (old_fd >= 0)
        {
            // safely released fd handle
            ::close(old_fd);
        }
    }

private:
    int fd_ = -1;
};