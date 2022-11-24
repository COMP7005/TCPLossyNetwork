
#include "../include/utils.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

struct tcpInfo {
    int seq;
    int ack;
    int fin;
    char data[NAME_MAX];
};

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

void write_stat(char* filename, char* data, int sentCnt, int recCnt)
{
    FILE* fp = fopen(filename, "a");
//    fprintf(fp, "%d, %d, %d, %s\n", recCnt * 100 / sentCnt, sentCnt, recCnt, data);
//    fprintf(fp, "%d, %d, %s\n", sentCnt, recCnt, data);
    fprintf(fp, "%d, %d\n", sentCnt, recCnt);
    fclose(fp);
}

void write_stat_proxy(char* filename, int sentCnt, int recCnt,
                        int sentDropCnt, int recDropCnt, struct tcpInfo *info)
{
    if (info->fin == 1)
        return;

    FILE* fp = fopen(filename, "a");
//    fprintf(fp, "%d, %d, %d, %d, %s\n", sentCnt, recCnt, sentDropCnt, recDropCnt, info->data);
    fprintf(fp, "%d, %d, %d, %d\n", sentCnt, recCnt, sentDropCnt, recDropCnt);
    fclose(fp);
}

//void write_stat_proxy(char* filename, char* data, int sentCnt, int recCnt, int sentDropCnt, int recDropCnt)
//{
//    FILE* fp = fopen(filename, "a");
//    fprintf(fp, "%d, %d, %d, %d, %s\n", sentCnt, recCnt, sentDropCnt, recDropCnt, data);
//    fclose(fp);
//}

//int get_percent(int sentCnt, int recCnt)
//{
//    return (recCnt * 100) / sentCnt;
//}
