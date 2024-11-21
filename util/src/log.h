#pragma once

#include "spdlog/spdlog.h"

// attempt to be compatible with both master and 1.x spdlog branches
#if __has_include("spdlog/sinks/stdout_color_sinks.h")
//#include "spdlog/sinks/stdout_color_sinks.h" // needed when on 1.x
#endif

/*
 spdlog::level, native spdlog loglevels:
    "trace", "debug", "info",  "warning", "error", "critical", "off"
*/

#define logformat "%H:%M:%S:%f %v"

extern std::shared_ptr<spdlog::logger> trace;

#define GREEN "\033[1;32m"
#define DARKGREEN "\033[32m"
#define WHITE "\033[1;30;37m"
#define RED "\033[1;31m"
#define CYAN "\033[1;36m"
#define YELLOW "\033[33;1m"
#define YELLOWONGREEN "\033[0;33m\033[42m"
#define WHITEONGREEN "\033[0;37m\033[42m"
#define WHITEONRED "\033[0;37m\033[41m"
#define RESET "\033[0m"

#define IMPORTANT WHITEONGREEN
