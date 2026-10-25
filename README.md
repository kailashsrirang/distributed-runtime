# distributed-runtime

A C++20 processing runtime for Linux, built incrementally toward distributed execution.

## Implemented so far

- CMake build setup with C++20 and compiler warnings.
- Separate Debug and Release build configurations.
- Reusable statistics library: count, minimum, maximum, and mean.
- Demo executable using a fixed set of readings.
- Tests for normal, single-value, negative, fractional, and empty input.

## Development

Edit source files on macOS through the UTM shared folder. Build and run inside Ubuntu.

## Build and run

```bash
cd /mnt/shared/distributed-runtime

cmake -S . -B ~/build/distributed-runtime/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build ~/build/distributed-runtime/debug

~/build/distributed-runtime/debug/runtime_demo
```

## Test

```bash
ctest --test-dir ~/build/distributed-runtime/debug --output-on-failure
```

## Next

Deterministic input generation and block splitting.