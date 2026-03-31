#pragma once

#include <cstdio>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace simpidlog {

namespace detail {

template <typename... Args>
std::string printf_format(const char* format, Args&&... args) {
    const int size = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...);
    if (size < 0) {
        throw std::runtime_error("failed to format log message");
    }

    std::vector<char> buffer(static_cast<std::size_t>(size) + 1U);
    const int written =
        std::snprintf(buffer.data(), buffer.size(), format, std::forward<Args>(args)...);
    if (written < 0) {
        throw std::runtime_error("failed to format log message");
    }

    return std::string(buffer.data(), static_cast<std::size_t>(written));
}

std::string info_message(const std::string& msg, bool output);
std::string warning_message(const std::string& msg, bool output);
std::string error_message(const std::string& msg, bool output);
std::string debug_message(const std::string& msg, bool output);

}  // namespace detail

void set_basedir(const std::string& basedir);
std::string get_basedir();

void wait_for_log_io();

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string info(const char* format, Args&&... args) {
    return detail::info_message(detail::printf_format(format, std::forward<Args>(args)...), true);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string info(bool output, const char* format, Args&&... args) {
    return detail::info_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string warning(const char* format, Args&&... args) {
    return detail::warning_message(detail::printf_format(format, std::forward<Args>(args)...), true);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string warning(bool output, const char* format, Args&&... args) {
    return detail::warning_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string error(const char* format, Args&&... args) {
    return detail::error_message(detail::printf_format(format, std::forward<Args>(args)...), true);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string error(bool output, const char* format, Args&&... args) {
    return detail::error_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string debug(const char* format, Args&&... args) {
    return detail::debug_message(detail::printf_format(format, std::forward<Args>(args)...), false);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
std::string debug(bool output, const char* format, Args&&... args) {
    return detail::debug_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

}  // namespace simpidlog
