#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <chrono>
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>
#include "log.hpp"

Log::Log() {
    // TODO: configure different level from file or something
    // console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);

    // file sink
    using namespace std::chrono;
    int64_t timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format("logs/{}.txt", timestamp), true);
    file_sink->set_level(spdlog::level::debug);

    spdlog::logger newLog("", {console_sink, file_sink});
    this->logger = std::make_unique<spdlog::logger>(newLog);
    logger->set_level(spdlog::level::debug);
}

#ifdef __APPLE__

void Log::debug(const std::string &msg, int lineNum, char const * funcName) {
    logger->debug(fmt::format("{}({}): {}", funcName, lineNum, msg));
}

void Log::info(const std::string &msg, int lineNum, char const * funcName) {
    logger->info(fmt::format("{}({}): {}", funcName, lineNum, msg));
}

void Log::warn(const std::string &msg, int lineNum, char const * funcName) {
    logger->warn(fmt::format("{}({}): {}", funcName, lineNum, msg));
}

void Log::error(const std::string &msg, int lineNum, char const * funcName) {
    logger->error(fmt::format("{}({}): {}", funcName, lineNum, msg));
}

void Log::vk_res(VkResult res, int lineNum, char const * funcName) {
    if (res != VK_SUCCESS) {
        error(fmt::format("VkResult is not success, result: {:s}", string_VkResult(res)), lineNum, funcName);
        assert(res == VK_SUCCESS);
    }
}

#else

void Log::debug(const std::string &msg, const std::source_location location) {
    logger->debug(fmt::format("{}({}): {}", location.function_name(), location.line(), msg));
}

void Log::info(const std::string &msg, const std::source_location location) {
    logger->info(fmt::format("{}({}): {}", location.function_name(), location.line(), msg));
}

void Log::warn(const std::string &msg, const std::source_location location) {
    logger->warn(fmt::format("{}({}): {}", location.function_name(), location.line(), msg));
}

void Log::error(const std::string &msg, const std::source_location location) {
    logger->error(fmt::format("{}({}): {}", location.function_name(), location.line(), msg));
}

void Log::vk_res(VkResult res, std::source_location location) {
    if (res != VK_SUCCESS) {
        error(fmt::format("VkResult is not success, result: {:s}", string_VkResult(res)), location);
        assert(res == VK_SUCCESS);
    }
}

#endif