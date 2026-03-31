#include "simpidlog/simpidlog.hpp"

int main() {
    simpidlog::set_basedir(".");

    simpidlog::info("%s", "training started");
    simpidlog::warning("%s", "learning rate looks high");
    simpidlog::error("%s", "dataset file is missing");
    simpidlog::debug("%s", "batch size = 32");

    simpidlog::wait_for_log_io();
    return 0;
}
