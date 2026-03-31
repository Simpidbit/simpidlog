#include "simpidlog/simpidlog.hpp"

#include "simpidlog/colorful.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdio>
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

namespace simpidlog {
namespace {

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

std::tm localtime_safe(const std::time_t time_value) {
    std::tm time_info{};
#if defined(_WIN32)
    localtime_s(&time_info, &time_value);
#else
    localtime_r(&time_value, &time_info);
#endif
    return time_info;
}

std::string format_datetime(
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

std::string format_log_timestamp(const std::chrono::system_clock::time_point& time_point) {
    const auto since_epoch = time_point.time_since_epoch();
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch) % std::chrono::seconds(1);
    return format_datetime(time_point, "%Y-%m-%d %H:%M:%S", 6, micros.count());
}

std::string format_directory_timestamp(const std::chrono::system_clock::time_point& time_point) {
    return format_datetime(time_point, "%Y-%m-%d_%H-%M-%S", 0, 0);
}

const char* level_prefix(MessageType type) {
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

std::string colorize(MessageType type, const std::string& message) {
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

std::string filename_for(MessageType type) {
    switch (type) {
        case MessageType::info:
            return "info.txt";
        case MessageType::warning:
            return "warning.txt";
        case MessageType::error:
            return "error.txt";
        case MessageType::debug:
            return "debug.txt";
        case MessageType::stop:
            return "";
    }
    return "";
}

struct LogFiles {
    std::ofstream info;
    std::ofstream warning;
    std::ofstream error;
    std::ofstream debug;
};

LogFiles open_log_files(const std::filesystem::path& directory) {
    std::filesystem::create_directories(directory);

    LogFiles files{
        std::ofstream(directory / "info.txt", std::ios::app),
        std::ofstream(directory / "warning.txt", std::ios::app),
        std::ofstream(directory / "error.txt", std::ios::app),
        std::ofstream(directory / "debug.txt", std::ios::app),
    };

    if (!files.info || !files.warning || !files.error || !files.debug) {
        throw std::runtime_error("failed to open log files");
    }

    return files;
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
                throw std::logic_error("simpidlog has been stopped by wait_for_log_io() and cannot accept new messages");
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
            LogFiles files = open_log_files(directory);

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
                    destination = &files.info;
                    break;
                case MessageType::warning:
                    destination = &files.warning;
                    break;
                case MessageType::error:
                    destination = &files.error;
                    break;
                case MessageType::debug:
                    destination = &files.debug;
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

LoggerState& logger_state() {
    static LoggerState state;
    return state;
}

std::string log_message(const std::string& msg, MessageType type, bool output) {
    logger_state().enqueue(msg, type, output);
    if (!output) {
        return colorize(type, msg);
    }
    return {};
}

}  // namespace

void set_basedir(const std::string& basedir) {
    logger_state().set_basedir(basedir);
}

std::string get_basedir() {
    return logger_state().get_basedir();
}

namespace detail {

std::string info_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::info, output);
}

std::string warning_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::warning, output);
}

std::string error_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::error, output);
}

std::string debug_message(const std::string& msg, bool output) {
    return log_message(msg, MessageType::debug, output);
}

}  // namespace detail

void wait_for_log_io() {
    logger_state().request_stop_and_join();
}

}  // namespace simpidlog
