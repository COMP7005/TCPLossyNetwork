
#ifndef TCPLOSSYNETWORK_UTIL_H
#define TCPLOSSYNETWORK_UTIL_H

#include <netinet/in.h>

in_port_t parse_port(const char *buff, int radix);
int atoi(const char* str);

#endif //TCPLOSSYNETWORK_UTIL_H
