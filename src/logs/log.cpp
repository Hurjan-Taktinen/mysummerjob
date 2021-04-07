#include "logs/log.h"
#include <spdlog/sinks/stdout_color_sinks.h>

void Log::init()
{
    m_Console = spdlog::stdout_color_mt("console");
    m_Console->set_pattern("[%^%l%$] %v");
}
