"""simpidlog package exports."""

from simpidlog import logger
from simpidlog import colorful
from simpidlog.logger import debug
from simpidlog.logger import error
from simpidlog.logger import get_basedir
from simpidlog.logger import info
from simpidlog.logger import set_basedir
from simpidlog.logger import wait_for_log_io
from simpidlog.logger import warning

__all__ = [
    "colorful",
    "debug",
    "error",
    "get_basedir",
    "info",
    "logger",
    "set_basedir",
    "wait_for_log_io",
    "warning",
]
