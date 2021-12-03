#pragma once

#include <spdlog/spdlog.h>

namespace Hunter::Compiler {
    class Logger {
    public:
        static void Init();
        inline static std::shared_ptr<spdlog::logger>& GetInstance() { return s_CoreLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
    };
}

#define COMPILER_ERROR(...) SPDLOG_LOGGER_ERROR(::Hunter::Compiler::Logger::GetInstance(), __VA_ARGS__)
#define COMPILER_WARN(...)  SPDLOG_LOGGER_WARN(::Hunter::Compiler::Logger::GetInstance(), __VA_ARGS__)
#define COMPILER_INFO(...)  SPDLOG_LOGGER_INFO(::Hunter::Compiler::Logger::GetInstance(), __VA_ARGS__)
#define COMPILER_DEBUG(...) SPDLOG_LOGGER_DEBUG(::Hunter::Compiler::Logger::GetInstance(), __VA_ARGS__)

