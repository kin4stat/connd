#pragma once
#include <filesystem>
#include <memory>

#ifdef _DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include "crtp_singleton.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#ifdef _DEBUG
#define LOG_TRACE(...)      SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)      SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)       SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARNING(...)    SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)      SPDLOG_ERROR(__VA_ARGS__)
#else
#define LOG_TRACE(...)      
#define LOG_DEBUG(...)      
#define LOG_INFO(...)       spdlog::info(__VA_ARGS__)
#define LOG_WARNING(...)    spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)      spdlog::error(__VA_ARGS__)
#endif

class logger : public Singleton<logger> {
public:
  logger() {
    constexpr auto logfile = "connd\\client.log";

    std::error_code ec{};

    std::filesystem::remove(logfile, ec);

    spdlogger = spdlog::basic_logger_mt("main_logger", logfile);
#ifdef _DEBUG
    spdlogger->set_level(spdlog::level::trace);
    spdlogger->flush_on(spdlog::level::trace);
    spdlogger->set_pattern("[%T] [thread: %t] [%l] - %v from [%@ in %!]");
#else
    spdlogger->set_level(spdlog::level::info);
    spdlogger->flush_on(spdlog::level::info);
    spdlogger->set_pattern("[%T] [%l] - %v");
#endif
    spdlog::set_default_logger(spdlogger);
  }

  std::shared_ptr<spdlog::logger> spdlogger{};
};
