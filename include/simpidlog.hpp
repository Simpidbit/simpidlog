#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace simpidlog {
namespace colorful {

inline std::string color_print_str(const std::string& s, int ccode) {
#if defined(__APPLE__) || defined(__linux__)
    return "\033[" + std::to_string(ccode) + "m" + s + "\033[0m";
#elif defined(_WIN32)
    return "`e[" + std::to_string(ccode) + "m" + s + "`e[0m";
#else
    return s;
#endif
}

inline std::string red_print_str(const std::string& s) {
    return color_print_str(s, 31);
}

inline std::string green_print_str(const std::string& s) {
    return color_print_str(s, 32);
}

inline std::string yellow_print_str(const std::string& s) {
    return color_print_str(s, 33);
}

inline std::string blue_print_str(const std::string& s) {
    return color_print_str(s, 34);
}

}  // namespace colorful

namespace detail {

enum class MessageType {
    stop,
    info,
    warning,
    error,
    debug,
};

struct Message {
    std::string text;
    MessageType type;
    bool output;
    std::string basedir;
    std::chrono::system_clock::time_point timestamp;
};

template <typename... Args>
inline std::string printf_format(const char* format, Args&&... args) {
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

inline std::tm localtime_safe(const std::time_t time_value) {
    std::tm time_info{};
#if defined(_WIN32)
    localtime_s(&time_info, &time_value);
#else
    localtime_r(&time_value, &time_info);
#endif
    return time_info;
}

inline std::string format_datetime(
    const std::chrono::system_clock::time_point& time_point,
    const char* format,
    int fractional_width,
    long long fractional_value) {
    const std::time_t time_value = std::chrono::system_clock::to_time_t(time_point);
    const std::tm time_info = localtime_safe(time_value);

    std::ostringstream stream;
    stream << std::put_time(&time_info, format);
    if (fractional_width > 0) {
        stream << '.' << std::setw(fractional_width) << std::setfill('0') << fractional_value;
    }
    return stream.str();
}

inline std::string format_log_timestamp(const std::chrono::system_clock::time_point& time_point) {
    const auto since_epoch = time_point.time_since_epoch();
    const auto micros =
        std::chrono::duration_cast<std::chrono::microseconds>(since_epoch) % std::chrono::seconds(1);
    return format_datetime(time_point, "%Y-%m-%d %H:%M:%S", 6, micros.count());
}

inline std::string format_directory_timestamp(
    const std::chrono::system_clock::time_point& time_point) {
    return format_datetime(time_point, "%Y-%m-%d_%H-%M-%S", 0, 0);
}

inline const char* level_prefix(MessageType type) {
    switch (type) {
        case MessageType::info:
            return "[ INFO    ] ";
        case MessageType::warning:
            return "[ WARNING ] ";
        case MessageType::error:
            return "[ ERROR   ] ";
        case MessageType::debug:
            return "[ DEBUG   ] ";
        case MessageType::stop:
            return "";
    }
    return "";
}

inline std::string colorize(MessageType type, const std::string& message) {
    switch (type) {
        case MessageType::info:
            return colorful::green_print_str(message);
        case MessageType::warning:
            return colorful::yellow_print_str(message);
        case MessageType::error:
            return colorful::red_print_str(message);
        case MessageType::debug:
            return colorful::blue_print_str(message);
        case MessageType::stop:
            return message;
    }
    return message;
}

class LoggerState {
public:
    LoggerState()
        : basedir_("."),
          start_time_(std::chrono::system_clock::now()),
          worker_(&LoggerState::run, this) {}

    ~LoggerState() {
        try {
            request_stop_and_join();
        } catch (...) {
        }
    }

    void set_basedir(const std::string& basedir) {
        std::lock_guard<std::mutex> lock(mutex_);
        basedir_ = basedir;
    }

    std::string get_basedir() {
        std::lock_guard<std::mutex> lock(mutex_);
        return basedir_;
    }

    void enqueue(const std::string& text, MessageType type, bool output) {
        Message message;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stop_requested_) {
                throw std::logic_error(
                    "simpidlog has been stopped by wait_for_log_io() and cannot accept new messages");
            }
            message = Message{text, type, output, basedir_, std::chrono::system_clock::now()};
            queue_.push(std::move(message));
        }
        condition_.notify_one();
    }

    void request_stop_and_join() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (joined_) {
            return;
        }

        if (!stop_requested_) {
            queue_.push(Message{"", MessageType::stop, false, basedir_, std::chrono::system_clock::now()});
            stop_requested_ = true;
            condition_.notify_one();
        }

        lock.unlock();
        if (worker_.joinable()) {
            worker_.join();
        }

        lock.lock();
        joined_ = true;
    }

private:
    void run() {
        for (;;) {
            Message message;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return !queue_.empty(); });
                message = std::move(queue_.front());
                queue_.pop();
            }

            const std::filesystem::path directory =
                std::filesystem::path(message.basedir) / "logs" / format_directory_timestamp(start_time_);
            std::filesystem::create_directories(directory);

            std::ofstream info_file(directory / "info.txt", std::ios::app);
            std::ofstream warning_file(directory / "warning.txt", std::ios::app);
            std::ofstream error_file(directory / "error.txt", std::ios::app);
            std::ofstream debug_file(directory / "debug.txt", std::ios::app);

            if (!info_file || !warning_file || !error_file || !debug_file) {
                throw std::runtime_error("failed to open log files");
            }

            if (message.type == MessageType::stop) {
                break;
            }

            const std::string full_message =
                std::string(level_prefix(message.type)) + "(" + format_log_timestamp(message.timestamp) + ") " + message.text;

            if (message.output) {
                std::cout << colorize(message.type, full_message) << std::endl;
            }

            std::ofstream* destination = nullptr;
            switch (message.type) {
                case MessageType::info:
                    destination = &info_file;
                    break;
                case MessageType::warning:
                    destination = &warning_file;
                    break;
                case MessageType::error:
                    destination = &error_file;
                    break;
                case MessageType::debug:
                    destination = &debug_file;
                    break;
                case MessageType::stop:
                    break;
            }

            if (destination == nullptr) {
                throw std::runtime_error("missing log destination");
            }

            (*destination) << full_message << '\n';
            destination->flush();
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<Message> queue_;
    std::string basedir_;
    std::chrono::system_clock::time_point start_time_;
    std::thread worker_;
    bool stop_requested_ = false;
    bool joined_ = false;
};

inline LoggerState& logger_state() {
    static LoggerState state;
    return state;
}

inline std::string log_message(const std::string& msg, MessageType type, bool output) {
    logger_state().enqueue(msg, type, output);
    if (!output) {
        return colorize(type, msg);
    }
    return {};
}

inline std::string info_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::info, output);
}

inline std::string warning_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::warning, output);
}

inline std::string error_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::error, output);
}

inline std::string debug_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::debug, output);
}

}  // namespace detail

inline void set_basedir(const std::string& basedir) {
    detail::logger_state().set_basedir(basedir);
}

inline std::string get_basedir() {
    return detail::logger_state().get_basedir();
}

inline void wait_for_log_io() {
    detail::logger_state().request_stop_and_join();
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string info(const char* format, Args&&... args) {
    return detail::info_message(detail::printf_format(format, std::forward<Args>(args)...), true);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string info(bool output, const char* format, Args&&... args) {
    return detail::info_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string warning(const char* format, Args&&... args) {
    return detail::warning_message(detail::printf_format(format, std::forward<Args>(args)...), true);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string warning(bool output, const char* format, Args&&... args) {
    return detail::warning_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string error(const char* format, Args&&... args) {
    return detail::error_message(detail::printf_format(format, std::forward<Args>(args)...), true);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string error(bool output, const char* format, Args&&... args) {
    return detail::error_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string debug(const char* format, Args&&... args) {
    return detail::debug_message(detail::printf_format(format, std::forward<Args>(args)...), false);
}

template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
inline std::string debug(bool output, const char* format, Args&&... args) {
    return detail::debug_message(detail::printf_format(format, std::forward<Args>(args)...), output);
}

}  // namespace simpidlog
