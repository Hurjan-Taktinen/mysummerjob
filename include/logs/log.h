#pragma once

#include "spdlog/spdlog.h"

#include <memory>

#define TRACE(...) Log::getLogger()->trace(__VA_ARGS__);
#define INFO(...) Log::getLogger()->info(__VA_ARGS__);
#define WARNING(...) Log::getLogger()->warn(__VA_ARGS__);
#define CRITICAL(...) Log::getLogger()->critical(__VA_ARGS__);

class Log
{
public:
    static void init();
    static std::shared_ptr<spdlog::logger> getLogger() { return m_Console; }

private:
    static inline std::shared_ptr<spdlog::logger> m_Console;
};
