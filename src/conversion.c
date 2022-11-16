#include "../include/conversion.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


in_port_t parse_port(const char *buff, int radix)
{
    char *end;
    long sl;
    in_port_t port;
    const char *msg;

    errno = 0;
    sl = strtol(buff, &end, radix);

    if(end == buff)
    {
        msg = "not a decimal number";
    }
    else if(*end != '\0')
    {
        msg = "%s: extra characters at end of input";
    }
    else if((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
    {
        msg = "out of range of type long";
    }
    else if(sl > UINT16_MAX)
    {
        msg = "greater than UINT16_MAX";
    }
    else if(sl < 0)
    {
        msg = "less than 0";
    }
    else
    {
        msg = NULL;
    }

    if(msg)
    {
        printf("[-]error %s\n", msg);
    }

    port = (in_port_t)sl;

    return port;
}


int atoi(const char* str)
{
    int num = 0;
    int i = 0;
    bool isNegetive = false;
    if(str[i] == '-'){
        isNegetive = true;
        i++;
    }
    while (str[i] && (str[i] >= '0' && str[i] <= '9')){
        num = num * 10 + (str[i] - '0');
        i++;
    }
    if(isNegetive) num = -1 * num;
    return num;
}
