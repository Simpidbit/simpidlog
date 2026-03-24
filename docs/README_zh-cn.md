[![English](https://img.shields.io/badge/English-README-0A7EA4)](../README.md)
[![简体中文](https://img.shields.io/badge/简体中文-README__zh--cn-2E8B57)](README_zh-cn.md)

# simpidlog

`simpidlog` 是一个轻量级 Python 日志库。它会通过后台线程异步写入日志文件，
同时可以按需在终端输出带颜色的日志信息。

## 功能特性

- 通过内部队列异步写日志
- 按 `info`、`warning`、`error`、`debug` 分别写入独立文件
- 支持彩色终端输出
- API 简单直接，不需要额外配置对象

## 安装

```bash
pip install simpidlog
```

本地开发安装方式：

```bash
pip install -e .
```

## 快速开始

```python
from simpidlog import debug, error, info, set_basedir, wait_for_log_io, warning

set_basedir(".")

info("training started")
warning("learning rate looks high")
error("dataset file is missing")
debug("batch size = 32")

wait_for_log_io()
```

当然你仍然可以使用 `from simpidlog import logger` 导入模块本身，不过现在这些常用
函数也已经在包的顶层直接导出了。

运行后会生成类似 `./logs/2026-03-24_12-34-56/` 的目录，包含：

- `info.txt`
- `warning.txt`
- `error.txt`
- `debug.txt`

每一行日志都带有时间戳和日志级别前缀。

## API 说明

### `set_basedir(basedir: str) -> None`

设置日志输出根目录，程序会在该目录下创建 `logs/` 文件夹。

### `get_basedir() -> str`

返回当前日志根目录。

### `info(msg: str, output: bool = True) -> None | str`

加入一条 info 日志。当 `output=True` 时，会在终端打印并写入 `info.txt`；当
`output=False` 时，日志仍然会写入文件，但函数会返回带颜色的字符串而不是直接打印。

### `warning(msg: str, output: bool = True) -> None | str`

加入一条 warning 日志，并写入 `warning.txt`。

### `error(msg: str, output: bool = True) -> None | str`

加入一条 error 日志，并写入 `error.txt`。

### `debug(msg: str, output: bool = False) -> None | str`

加入一条 debug 日志，并写入 `debug.txt`。默认不会输出到终端。

### `wait_for_log_io() -> None`

刷新队列中的日志并停止后台日志线程。如果你希望程序退出前确保所有待写日志都已经落盘，
应在结束前调用这个函数。

## 颜色辅助函数

`simpidlog.colorful` 模块还提供以下字符串着色函数：

- `color_print_str(s, ccode)`
- `red_print_str(s)`
- `green_print_str(s)`
- `yellow_print_str(s)`
- `blue_print_str(s)`

这些函数只返回着色后的字符串，本身不会直接打印。

## 注意事项

- 日志目录按后台 worker 启动时的时间戳分组
- `simpidlog.logger` 被导入时，后台线程会自动启动
- 调用 `wait_for_log_io()` 之后，不应在同一进程中继续写日志，除非你自己补充重启线程的逻辑
