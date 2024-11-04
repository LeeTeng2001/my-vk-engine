#pragma once

#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

#include <source_location>

namespace luna {
    class Log {
       private:
        std::unique_ptr<spdlog::logger> logger;

       public:
        Log();

        // std::source_location::current() is nice BUT it's too freaking verbose
        // for our case
        void debug(const std::string &msg, int lineNum = __builtin_LINE(),
                   char const *funcName = __builtin_FUNCTION());
        void info(const std::string &msg, int lineNum = __builtin_LINE(),
                  char const *funcName = __builtin_FUNCTION());
        void warn(const std::string &msg, int lineNum = __builtin_LINE(),
                  char const *funcName = __builtin_FUNCTION());
        void error(const std::string &msg, int lineNum = __builtin_LINE(),
                   char const *funcName = __builtin_FUNCTION());
        void vk_res(VkResult res, int lineNum = __builtin_LINE(),
                    char const *funcName = __builtin_FUNCTION());
    };

    // global logger handler
    // https://stackoverflow.com/questions/1008019/how-do-you-implement-the-singleton-design-pattern
    class SLog {
       public:
        SLog() = delete;
        SLog(const SLog &) = delete;
        SLog &operator=(const SLog &) = delete;

        static Log *get() {
            static Log instance{};
            return &instance;
        }
    };

    // Lua Log wrapper
    class LuaLog {
       public:
        LuaLog() = default;
        void debug(const std::string &msg) {
            auto l = SLog::get();
            l->debug(msg, -1, "luaDebug");
        };
        void info(const std::string &msg) {
            auto l = SLog::get();
            l->info(msg, -1, "luaInfo");
        };
        void warn(const std::string &msg) {
            auto l = SLog::get();
            l->warn(msg, -1, "luaWarn");
        };
        void error(const std::string &msg) {
            auto l = SLog::get();
            l->error(msg, -1, "luaError");
        };
    };
}  // namespace luna