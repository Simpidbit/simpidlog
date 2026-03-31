# simpidlog-cpp

`simpidlog-cpp` is a C++17 translation of the Python `simpidlog` library. It writes
messages from a background worker thread into per-level log files while optionally
printing colored output to the terminal.

## Features

- asynchronous log writing through an internal queue
- separate log files for `info`, `warning`, `error`, and `debug`
- optional colored terminal output
- small function-based API similar to the Python version

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Quick Start

```cpp
#include "simpidlog.hpp"

int main() {
    simpidlog::set_basedir(".");

    simpidlog::info("%s", "training started");
    simpidlog::info("epoch=%d loss=%.4f", 7, 0.2513);
    simpidlog::warning("%s", "learning rate looks high");
    simpidlog::error("%s", "dataset file is missing");
    simpidlog::debug("%s", "batch size = 32");
    simpidlog::debug(true, "worker=%d queue=%zu", 2, std::size_t{4});

    simpidlog::wait_for_log_io();
}
```

This creates a directory like `./logs/2026-03-31_12-34-56/` containing:

- `info.txt`
- `warning.txt`
- `error.txt`
- `debug.txt`

Each line is timestamped and prefixed with its log level.

## API

- `void set_basedir(const std::string& basedir)`
- `std::string get_basedir()`
- `std::string info(const char* format, ...)`
- `std::string info(bool output, const char* format, ...)`
- `std::string warning(const char* format, ...)`
- `std::string warning(bool output, const char* format, ...)`
- `std::string error(const char* format, ...)`
- `std::string error(bool output, const char* format, ...)`
- `std::string debug(const char* format, ...)`
- `std::string debug(bool output, const char* format, ...)`
- `void wait_for_log_io()`

These functions use `printf`-style formatting:

- `simpidlog::info("epoch=%d loss=%.4f", epoch, loss)`
- `simpidlog::warning(false, "retry in %d seconds", seconds)`
- `simpidlog::error("missing file: %s", filename.c_str())`
- `simpidlog::debug(true, "worker=%d queue=%zu", worker_id, queue_size)`

When `output` is `false`, the formatted message is still queued for file output
and the colored string is returned instead of being printed immediately.

## Color Helpers

The `simpidlog/colorful.hpp` header exposes:

- `simpidlog::colorful::color_print_str(s, ccode)`
- `simpidlog::colorful::red_print_str(s)`
- `simpidlog::colorful::green_print_str(s)`
- `simpidlog::colorful::yellow_print_str(s)`
- `simpidlog::colorful::blue_print_str(s)`

## Notes

- log files are grouped by the worker start timestamp
- the worker thread starts automatically on first library use
- after calling `wait_for_log_io()`, further logging in the same process is not supported
