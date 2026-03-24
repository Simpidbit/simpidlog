"""Threaded file logger with optional colored console output."""

import os
from threading import Thread
from queue import Queue
from time import time as get_timestamp
from datetime import datetime

from tqdm import tqdm

from simpidlog import colorful

_simpidlog_basedir_g: str = "."


def set_basedir(basedir: str) -> None:
    """Set the root directory used to create the `logs/` folder."""

    global _simpidlog_basedir_g
    _simpidlog_basedir_g = basedir


def get_basedir() -> str:
    """Return the current root directory used for log output."""

    global _simpidlog_basedir_g
    return _simpidlog_basedir_g


_SIMPIDLOG_STOP_CODE = -1
_SIMPIDLOG_INFO_CODE = 0
_SIMPIDLOG_WARNING_CODE = 1
_SIMPIDLOG_ERROR_CODE = 2
_SIMPIDLOG_DEBUG_CODE = 3


class _Message:
    def __init__(
        self,
        msg: str,
        msg_type: int,
        output: bool,
        basedir: str = ".",
        timestamp: float | None = None,
    ) -> None:
        self.msg: str = msg
        self.msg_type: int = msg_type
        self.output: bool = output
        if timestamp is None:
            self.timestamp: float = get_timestamp()
        else:
            self.timestamp: float = timestamp
        self.basedir = basedir


_simpidlog_msg_queue_g: Queue[_Message] = Queue(maxsize=0)


def worker() -> None:
    """Consume queued log messages and write them to disk."""

    global _simpidlog_msg_queue_g
    start_time = get_timestamp()
    start_time_str = datetime.fromtimestamp(start_time).strftime("%Y-%m-%d_%H-%M-%S")

    while True:
        msgobj = _simpidlog_msg_queue_g.get(block=True)

        dir_name = f"{msgobj.basedir}/logs/{start_time_str}"
        os.makedirs(dir_name, exist_ok=True)
        info_file = open(f"{dir_name}/info.txt", "at")
        warning_file = open(f"{dir_name}/warning.txt", "at")
        error_file = open(f"{dir_name}/error.txt", "at")
        debug_file = open(f"{dir_name}/debug.txt", "at")

        try:
            timestr = datetime.fromtimestamp(msgobj.timestamp).strftime(
                "%Y-%m-%d %H:%M:%S.%f"
            )
            msgstr = f"({timestr}) {msgobj.msg}"
            if msgobj.msg_type == _SIMPIDLOG_INFO_CODE:
                msgstr = "[ INFO    ] " + msgstr
                if msgobj.output:
                    tqdm.write(colorful.green_print_str(msgstr))
                info_file.write(msgstr + "\n")
                info_file.flush()
            elif msgobj.msg_type == _SIMPIDLOG_WARNING_CODE:
                msgstr = "[ WARNING ] " + msgstr
                if msgobj.output:
                    tqdm.write(colorful.yellow_print_str(msgstr))
                warning_file.write(msgstr + "\n")
                warning_file.flush()
            elif msgobj.msg_type == _SIMPIDLOG_ERROR_CODE:
                msgstr = "[ ERROR   ] " + msgstr
                if msgobj.output:
                    tqdm.write(colorful.red_print_str(msgstr))
                error_file.write(msgstr + "\n")
                error_file.flush()
            elif msgobj.msg_type == _SIMPIDLOG_DEBUG_CODE:
                msgstr = "[ DEBUG   ] " + msgstr
                if msgobj.output:
                    tqdm.write(colorful.blue_print_str(msgstr))
                debug_file.write(msgstr + "\n")
                debug_file.flush()
            elif msgobj.msg_type == _SIMPIDLOG_STOP_CODE:
                info_file.close()
                warning_file.close()
                error_file.close()
                debug_file.close()
                break
        except:
            info_file.close()
            warning_file.close()
            error_file.close()
            debug_file.close()
            raise


_simpidlog_working_thread_g: Thread = Thread(target=worker, daemon=True)
_simpidlog_working_thread_g.start()


def wait_for_log_io() -> None:
    """Flush queued logs and stop the background logging thread.

    Call this before program exit when you need to guarantee that every queued
    log message has been written to disk.
    """

    _simpidlog_msg_queue_g.put(
        _Message(str(), _SIMPIDLOG_STOP_CODE, False, _simpidlog_basedir_g)
    )
    _simpidlog_working_thread_g.join()


def info(
    msg: str,
    output: bool = True,
) -> None | str:
    """Queue an info message.

    When `output` is `True`, the message is printed to the console and written
    to `info.txt`. When `output` is `False`, the message is still logged to the
    file and the colored string is returned instead of being printed.
    """

    global _simpidlog_msg_queue_g
    _simpidlog_msg_queue_g.put(
        _Message(msg, _SIMPIDLOG_INFO_CODE, output, _simpidlog_basedir_g)
    )
    if not output:
        return colorful.green_print_str(msg)


def warning(
    msg: str,
    output: bool = True,
) -> None | str:
    """Queue a warning message.

    When `output` is `True`, the message is printed to the console and written
    to `warning.txt`. When `output` is `False`, the message is still logged to
    the file and the colored string is returned instead of being printed.
    """

    global _simpidlog_msg_queue_g
    _simpidlog_msg_queue_g.put(
        _Message(msg, _SIMPIDLOG_WARNING_CODE, output, _simpidlog_basedir_g)
    )
    if not output:
        return colorful.yellow_print_str(msg)


def error(
    msg: str,
    output: bool = True,
) -> None | str:
    """Queue an error message.

    When `output` is `True`, the message is printed to the console and written
    to `error.txt`. When `output` is `False`, the message is still logged to the
    file and the colored string is returned instead of being printed.
    """

    global _simpidlog_msg_queue_g
    _simpidlog_msg_queue_g.put(
        _Message(msg, _SIMPIDLOG_ERROR_CODE, output, _simpidlog_basedir_g)
    )
    if not output:
        return colorful.red_print_str(msg)


def debug(
    msg: str,
    output: bool = False,
) -> None | str:
    """Queue a debug message.

    Debug logging defaults to `output=False`, so debug messages are written to
    `debug.txt` without being printed unless you opt in.
    """

    global _simpidlog_msg_queue_g
    _simpidlog_msg_queue_g.put(
        _Message(msg, _SIMPIDLOG_DEBUG_CODE, output, _simpidlog_basedir_g)
    )
    if not output:
        return colorful.blue_print_str(msg)
