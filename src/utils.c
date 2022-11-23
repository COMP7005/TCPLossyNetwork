
#include "../include/utils.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

void create_directory(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

char * append(char * string1, char * string2) {
    char * result = NULL;
    asprintf(&result, "%s%s", string1, string2);
    return result;
}

void write_to_file(FILE *fp, char* data, int counter)
{
//    fp = fopen(filename, "a");
//    fprintf(fp, "[%d]: %s\n", counter, data);
    fprintf(fp, "%d, %s\n", counter, data);

//    fclose(fp);
}

void write_to_file_tmp(char* filename, char* data, int counter)
{
    FILE* fp = fopen(filename, "a");
    fprintf(fp, "%d, %s\n", counter, data);
    fclose(fp);
}

void write_stat(char* filename, char* data, int sentCnt, int recCnt)
{
    FILE* fp = fopen(filename, "a");
    fprintf(fp, "%d, %d, %d, %s\n", recCnt * 100 / sentCnt, sentCnt, recCnt, data);
    fclose(fp);
}

//int get_percent(int sentCnt, int recCnt)
//{
//    return (recCnt * 100) / sentCnt;
//}
