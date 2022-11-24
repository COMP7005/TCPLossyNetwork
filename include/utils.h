//
// Created by Hansol Lee on 2022-11-12.
//

#ifndef TCPLOSSYNETWORK_UTIL_H
#define TCPLOSSYNETWORK_UTIL_H

void create_directory(const char *dir);
char * append(char * string1, char * string2);
void write_stat(char* filename, char* data, int sentCnt, int recCnt);
void write_stat_proxy(char* filename, int sentCnt, int recCnt, int sentDropCnt, int recDropCnt, struct tcpInfo *info);
//int get_percent(int sentCnt, int recCnt);

#endif //TCPLOSSYNETWORK_UTIL_H
