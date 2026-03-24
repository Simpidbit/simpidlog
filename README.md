[![English](https://img.shields.io/badge/English-README-0A7EA4)](README.md)
[![简体中文](https://img.shields.io/badge/简体中文-README__zh--cn-2E8B57)](docs/README_zh-cn.md)

# simpidlog

`simpidlog` is a small Python logging helper that writes messages from a
background thread into per-level log files while optionally printing colored
messages to the console.

## Features

- asynchronous log writing through an internal queue
- separate log files for `info`, `warning`, `error`, and `debug`
- optional colored terminal output
- simple API with no configuration objects

## Installation

```bash
pip install simpidlog
```

For local development:

```bash
pip install -e .
```

## Quick Start

```python
from simpidlog import debug, error, info, set_basedir, wait_for_log_io, warning

set_basedir(".")

info("training started")
warning("learning rate looks high")
error("dataset file is missing")
debug("batch size = 32")

wait_for_log_io()
```

You can still import the module itself with `from simpidlog import logger`, but
the common logging helpers are now also re-exported at the package level.

This creates a directory like `./logs/2026-03-24_12-34-56/` containing:

- `info.txt`
- `warning.txt`
- `error.txt`
- `debug.txt`

Each line is timestamped and prefixed with its log level.

## API

### `set_basedir(basedir: str) -> None`

Sets the root directory where the `logs/` folder will be created.

### `get_basedir() -> str`

Returns the current log base directory.

### `info(msg: str, output: bool = True) -> None | str`

Queues an info message. When `output=True`, the message is printed and written
to `info.txt`. When `output=False`, it is still written to disk and the colored
string is returned instead of being printed.

### `warning(msg: str, output: bool = True) -> None | str`

Queues a warning message and writes it to `warning.txt`.

### `error(msg: str, output: bool = True) -> None | str`

Queues an error message and writes it to `error.txt`.

### `debug(msg: str, output: bool = False) -> None | str`

Queues a debug message and writes it to `debug.txt`. Debug output is not printed
by default.

### `wait_for_log_io() -> None`

Flushes queued messages and stops the background logging thread. Call this near
program exit if you need to make sure every pending message has been written.

## Color Helpers

The `simpidlog.colorful` module also exposes string helpers:

- `color_print_str(s, ccode)`
- `red_print_str(s)`
- `green_print_str(s)`
- `yellow_print_str(s)`
- `blue_print_str(s)`

These helpers return colored strings and do not print by themselves.

## Notes

- log files are grouped by the timestamp of the worker start time
- the background worker is started automatically when `simpidlog.logger` is
  imported
- after calling `wait_for_log_io()`, do not queue more messages in the
  same process unless you add your own restart logic
