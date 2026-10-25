## Implemented
- C++20 build using CMake and Ninja.
- Compiler warnings and separate Debug/Release configurations.
- CTest integration.
- Reusable runtime_core static library.
- Sequential statistics: count, minimum, maximum, and mean.
- Deterministic sensor-data generation.
- Block splitting without copying readings.
- Demo processing numbered blocks sequentially.
- Tests for statistics, repeatability, block boundaries, and input coverage.
- Added file descriptor ownership see below

## File descriptor ownership
- Added header-only UniqueFd.
- Destruction and reset close the owned descriptor.
- Copying is prohibited; moving transfers ownership.
- get() borrows access; release() transfers cleanup responsibility.
- Tests cover moves, replacement, release, destruction, and repeated reset.
- Debug and Release tests passed.
- Next: a blocking TCP connection between two processes.