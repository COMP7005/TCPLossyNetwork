//
// Created by Hansol Lee on 2022-11-05.
//

#ifndef TCPLOSSYNETWORK_UTIL_H
#define TCPLOSSYNETWORK_UTIL_H

#include <stddef.h>

_Noreturn void fatal_errno(const char *file, const char *func, size_t line, int err_code, int exit_code);
_Noreturn void error_message(const char *file, const char *func, size_t line, const char *msg, int exit_code);

#endif //TCPLOSSYNETWORK_UTIL_H