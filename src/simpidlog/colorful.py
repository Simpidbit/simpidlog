"""Helpers for wrapping strings with ANSI color escape codes."""

from platform import system as get_system


def color_print_str(
    s: str,
    ccode: int,
) -> str:
    """Return `s` wrapped in the escape sequence for `ccode` when supported."""

    system = get_system()
    if system == "Darwin" or system == "Linux":
        return f"\033[{ccode}m{s}\033[0m"
    elif system == "Windows":
        return f"`e[{ccode}m{s}`e[0m"
    else:
        return s


def red_print_str(s: str) -> str:
    """Return `s` formatted in red."""

    return color_print_str(s, 31)


def green_print_str(s: str) -> str:
    """Return `s` formatted in green."""

    return color_print_str(s, 32)


def yellow_print_str(s: str) -> str:
    """Return `s` formatted in yellow."""

    return color_print_str(s, 33)


def blue_print_str(s: str) -> str:
    """Return `s` formatted in blue."""

    return color_print_str(s, 34)
