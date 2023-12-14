#pragma once

#include <spdlog/spdlog.h>
#include <source_location>
#include <vulkan/vulkan_core.h>

class Log {
private:
    std::unique_ptr<spdlog::logger> logger;
public:
    Log();

#ifdef __APPLE__  // source_location is not usable at current MacOS state
    void debug(const std::string& msg, int lineNum = __builtin_LINE(), char const * funcName = __builtin_FUNCTION());
    void info(const std::string& msg, int lineNum = __builtin_LINE(), char const * funcName = __builtin_FUNCTION());
    void warn(const std::string& msg, int lineNum = __builtin_LINE(), char const * funcName = __builtin_FUNCTION());
    void error(const std::string& msg, int lineNum = __builtin_LINE(), char const * funcName = __builtin_FUNCTION());
    void vk_res(VkResult res, int lineNum = __builtin_LINE(), char const * funcName = __builtin_FUNCTION());
#else
    void debug(const std::string& msg, std::source_location location = std::source_location::current());
    void info(const std::string& msg, std::source_location location = std::source_location::current());
    void warn(const std::string& msg, std::source_location location = std::source_location::current());
    void error(const std::string& msg, std::source_location location = std::source_location::current());
    void vk_res(VkResult res, std::source_location location = std::source_location::current());
#endif
};

// global logger handler
// https://stackoverflow.com/questions/1008019/how-do-you-implement-the-singleton-design-pattern
class SLog {
public:
    SLog() = delete;
    SLog(const SLog &) = delete;
    SLog &operator=(const SLog &) = delete;

    static Log* get() {
        static Log instance{};
        return &instance;
    }
};
