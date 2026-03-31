#pragma once

#include <string>

namespace simpidlog::colorful {

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

}  // namespace simpidlog::colorful
