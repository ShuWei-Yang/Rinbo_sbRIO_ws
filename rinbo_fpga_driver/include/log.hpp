#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <mutex>

extern std::ofstream log_file;
extern std::mutex log_mtx;

#define LOG(msg) \
    do { \
        std::lock_guard<std::mutex> lock(log_mtx); \
        if (log_file.is_open()) { \
            log_file << msg << std::endl; \
            log_file.flush(); \
        } \
    } while (0)

#define LOG_PTR(name, ptr) \
    LOG("[PTR] " << name << " = " << (void*)(ptr))

#define LOG_INT(name, val) \
    LOG("[INT] " << name << " = " << (int)(val))

#define LOG_SIZE(name, val) \
    LOG("[SIZE] " << name << " = " << (size_t)(val))

#endif
