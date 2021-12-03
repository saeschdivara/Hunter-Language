#include "logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Hunter::Compiler {

    std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;

    void Logger::Init() {
        spdlog::set_pattern("%^[%T] [source %s] [function %!] [line %#]: %v%$");

        s_CoreLogger = spdlog::stdout_color_mt("CORE");
        s_CoreLogger->set_level(spdlog::level::trace);
    }
}