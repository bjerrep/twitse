#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

/*
 spdlog::level, native spdlog loglevels:
    "trace", "debug", "info",  "warning", "error", "critical", "off"
*/

#define logformat "%H:%M:%S:%f %v"

extern std::shared_ptr<spdlog::logger> trace;

#define WHITEONGREEN "\033[48;5;22m"
#define GREEN "\033[1;32m"
#define DARKGREEN "\033[32m"
#define WHITE "\033[1;30;37m"
#define RED "\033[1;31m"
#define CYAN "\033[1;36m"
#define YELLOW "\033[33;1m"
#define RESET "\033[0m"

#define IMPORTANT WHITEONGREEN
