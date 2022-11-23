//
// Created by Hansol Lee on 2022-11-12.
//

#ifndef TCPLOSSYNETWORK_UTIL_H
#define TCPLOSSYNETWORK_UTIL_H

void create_directory(const char *dir);
char * append(char * string1, char * string2);
void write_to_file(FILE *fp, char* data, int counter);
void write_to_file_tmp(char* filename, char* data, int counter);
void write_stat(char* filename, char* data, int sentCnt, int recCnt);
//int get_percent(int sentCnt, int recCnt);

#endif //TCPLOSSYNETWORK_UTIL_H
