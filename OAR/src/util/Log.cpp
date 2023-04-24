#include "pch/pch.h"

#include "util/Log.h"

std::shared_ptr<spdlog::logger> Log::s_core_logger;

void Log::Init() {
	spdlog::set_pattern("%^[%T] %n: %v%$");
	s_core_logger = spdlog::stdout_color_mt("OAR");
	s_core_logger->set_level(spdlog::level::info);
}