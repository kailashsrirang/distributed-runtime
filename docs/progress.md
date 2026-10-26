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

## Blocking TCP checkpoint
- Added coordinator and worker executables.
- Coordinator listens on loopback port 5000.
- Verified a one-byte request/reply exchange.
- Socket descriptors use UniqueFd ownership.
- Checked worker failure when no coordinator is listening.
- No task data, timeouts, or automatic TCP tests yet.
- Next: framed messages and complete-buffer send/receive operations.

## Protocol header
- Added explicit 12-byte header encoding and decoding.
- Fields: version, message type, reserved bytes, task ID, payload length.
- Multibyte fields use big-endian order.
- Payload limit: 64 KiB.
- Tests check exact wire bytes, invalid headers, and size boundaries.
- Debug and Release tests passed.
- Header is not connected to socket transport yet.
- Next: complete-buffer socket I/O and framed message exchange.

## Framed message transport
- Added blocking complete-buffer send and receive loops.
- Messages own their payload bytes.
- Header validation precedes payload allocation.
- Clean EOF returns an empty optional; truncated messages throw.
- Coordinator and worker exchange a framed demonstration payload.
- Added stream tests for consecutive messages, split writes, and EOF.
- Debug and Release tests passed.
- No socket deadlines yet; an unresponsive peer can block a process.
- Next: encode sensor readings and statistics for actual remote processing.

## Remote computation
- Task payload: 32-bit count followed by binary64 readings.
- Result payload: 32-bit count, minimum, maximum, and mean.
- Multibyte fields use big-endian order.
- Maximum task size: 8191 readings.
- Empty tasks and non-finite values are rejected.
- Verified one task through coordinator and worker processes.
- Coordinator checks result type, task ID, and statistics.
- Debug and Release tests passed.
- Still one task per connection; no worker thread pool yet.
- Next: process multiple numbered blocks over one connection.

## Multiple-block processing
- Coordinator sends one task at a time over a persistent connection.
- Results are validated and stored in task order.
- Verified blocks of 4, 4, and 2 readings.
- Worker processes tasks until EOF between messages.
- Coordinator closes the connection after receiving all results.
- Added a process-level integration test with timeouts.
- Debug and Release tests passed.
- Still one worker, one computation thread, and one task in flight.
- Next: bounded queue with close semantics for the worker thread pool.