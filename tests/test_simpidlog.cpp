#include "simpidlog/colorful.hpp"
#include "simpidlog/simpidlog.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    assert(input.good());
    std::ostringstream stream;
    stream << input.rdbuf();
    return stream.str();
}

}  // namespace

int main() {
    const auto unique_suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::filesystem::path base_dir =
        std::filesystem::temp_directory_path() / ("simpidlog-cpp-test-" + unique_suffix);

    std::filesystem::create_directories(base_dir);

    simpidlog::set_basedir(base_dir.string());
    assert(simpidlog::get_basedir() == base_dir.string());

    const std::string info_colored = simpidlog::info(false, "%s", "training started");
    const std::string warning_colored = simpidlog::warning(false, "%s", "learning rate looks high");
    const std::string error_colored = simpidlog::error(false, "%s", "dataset file is missing");
    const std::string debug_colored = simpidlog::debug(false, "%s", "batch size = 32");
    const std::string formatted_info = simpidlog::info(false, "epoch=%d loss=%.2f", 7, 0.25);
    const std::string formatted_warning = simpidlog::warning(false, "retry in %d seconds", 3);
    const std::string formatted_error = simpidlog::error(false, "cannot open %s", "dataset.csv");
    const std::string formatted_debug = simpidlog::debug("worker=%d step=%d", 2, 9);

    assert(info_colored == simpidlog::colorful::green_print_str("training started"));
    assert(warning_colored == simpidlog::colorful::yellow_print_str("learning rate looks high"));
    assert(error_colored == simpidlog::colorful::red_print_str("dataset file is missing"));
    assert(debug_colored == simpidlog::colorful::blue_print_str("batch size = 32"));
    assert(formatted_info == simpidlog::colorful::green_print_str("epoch=7 loss=0.25"));
    assert(formatted_warning == simpidlog::colorful::yellow_print_str("retry in 3 seconds"));
    assert(formatted_error == simpidlog::colorful::red_print_str("cannot open dataset.csv"));
    assert(formatted_debug == simpidlog::colorful::blue_print_str("worker=2 step=9"));

    simpidlog::wait_for_log_io();

    const std::filesystem::path logs_root = base_dir / "logs";
    assert(std::filesystem::exists(logs_root));

    std::filesystem::path session_dir;
    for (const auto& entry : std::filesystem::directory_iterator(logs_root)) {
        if (entry.is_directory()) {
            session_dir = entry.path();
            break;
        }
    }

    assert(!session_dir.empty());

    const std::string info_file = read_file(session_dir / "info.txt");
    const std::string warning_file = read_file(session_dir / "warning.txt");
    const std::string error_file = read_file(session_dir / "error.txt");
    const std::string debug_file = read_file(session_dir / "debug.txt");

    assert(info_file.find("[ INFO    ]") != std::string::npos);
    assert(info_file.find("training started") != std::string::npos);
    assert(info_file.find("epoch=7 loss=0.25") != std::string::npos);
    assert(warning_file.find("[ WARNING ]") != std::string::npos);
    assert(warning_file.find("learning rate looks high") != std::string::npos);
    assert(warning_file.find("retry in 3 seconds") != std::string::npos);
    assert(error_file.find("[ ERROR   ]") != std::string::npos);
    assert(error_file.find("dataset file is missing") != std::string::npos);
    assert(error_file.find("cannot open dataset.csv") != std::string::npos);
    assert(debug_file.find("[ DEBUG   ]") != std::string::npos);
    assert(debug_file.find("batch size = 32") != std::string::npos);
    assert(debug_file.find("worker=2 step=9") != std::string::npos);

    bool threw_after_stop = false;
    try {
        (void)simpidlog::info(false, "%s", "should fail after stop");
    } catch (const std::logic_error&) {
        threw_after_stop = true;
    }
    assert(threw_after_stop);

    std::filesystem::remove_all(base_dir);
    return 0;
}
