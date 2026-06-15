#include "log.hpp"

std::mutex log_mtx;
std::ofstream log_file(
    "/tmp/fpga_server_debug.log",   // ← 強烈建議用 /tmp
    std::ios::out | std::ios::app
);
