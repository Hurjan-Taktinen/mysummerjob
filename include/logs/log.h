#pragma once

#include "spdlog/spdlog.h"

#include <memory>
#include <string>
#include <mutex>

// GL-prefix means global.. had to include that to prevent conflicts

#define GLTRACE(...) logs::Log::getLogger()->trace(__VA_ARGS__);
#define GLINFO(...) logs::Log::getLogger()->info(__VA_ARGS__);
#define GLWARNING(...) logs::Log::getLogger()->warn(__VA_ARGS__);
#define GLCRITICAL(...) logs::Log::getLogger()->critical(__VA_ARGS__);

namespace logs
{
using Logger = std::shared_ptr<spdlog::logger>;

class Log final
{
public:
    // Inialize global logger
    static void init();

    // Logging macros uses these
    [[nodiscard]] static std::shared_ptr<spdlog::logger> getLogger();

    // Create logger for individual thread/module where it benefits to have
    // static name attached to logging context
    [[nodiscard]] static std::shared_ptr<spdlog::logger> create(
            const std::string& name);

private:
    static inline std::shared_ptr<spdlog::logger> m_Console;
    static inline std::mutex m_Mutex;
};

} // namespace logs
