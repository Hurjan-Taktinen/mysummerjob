#include "logs/log.h"
#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>

namespace logs
{
void Log::init()
{
    m_Console = spdlog::stdout_color_mt("global");
    m_Console->set_pattern("[%^%l%$] %v");
}

std::shared_ptr<spdlog::logger> Log::getLogger()
{
    return m_Console;
}

std::shared_ptr<spdlog::logger> Log::create(const std::string& name)
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    auto log = spdlog::stdout_color_mt(name);
    log->set_pattern("[%H:%M:%S] [%t] [%n] [%^%l%$] || %v");
    return log;
}

} // namespace logs
