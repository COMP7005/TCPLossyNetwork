#include "../conversion.c"
#include "../error.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

typedef struct fileInfo {
    char fname[NAME_MAX + 1];
    long fsize;
} file_info;

struct clientOptions
{
    int file_cnt;
    char *server_ip;
    in_port_t port;
};

static void send_file(FILE *fp, char *fname, int sockfd);
static void options_init(struct clientOptions *opts);
static void parse_client_arguments(int argc, char *argv[], struct clientOptions *opts);

#define DEFAULT_PORT 5000
#define SIZE 1024
const char* files[20];

int main (int argc, char *argv[]) {
    struct clientOptions opts;

    options_init(&opts);
    parse_client_arguments(argc, argv, &opts);

    int clientSocket, ret;
    struct sockaddr_in serverAddr;

    FILE *fp;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);
    }
    printf("[+]Client socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(opts.port);
    serverAddr.sin_addr.s_addr = inet_addr(opts.server_ip);

    ret = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret < 0) {
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);
    }
    printf("[+]Connect to Server.\n");

    for (int i = 0; i < opts.file_cnt; i++){
        fp = fopen(files[i], "r");
        if (fp == NULL) {
            error_message(__FILE__, __func__ , __LINE__, "Can't read a file", 2);
        }
        char *fname = (char*) malloc(sizeof(files[i])+1);
        strcpy(fname, files[i]);

        printf("[+]Sending: %s....\n", fname);
        send_file(fp, fname, clientSocket);
    }
    // Indicates end of files.
    file_info info;
    strcpy(info.fname, "");
    info.fsize = 0;
    write(clientSocket, &info, sizeof(info));

    close(clientSocket);
    printf("[+]File data send successfully.\n");

    return EXIT_SUCCESS;
}

static void options_init(struct clientOptions *opts)
{
    memset(opts, 0, sizeof(struct clientOptions));
    opts->server_ip = "127.0.0.1"; //default localhost
    opts->port  = DEFAULT_PORT;
}

void send_file(FILE *file, char *fname, int clientSocket) {
    char* buffer;
    long numbytes;

    fseek(file, 0L, SEEK_END);
    numbytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    buffer = (char*)calloc(numbytes, sizeof(char));

    if (buffer == NULL)
        EXIT_FAILURE;

    fread(buffer, sizeof(char), numbytes, file);

    file_info info;
    strcpy(info.fname, fname);
    info.fsize = numbytes;

    write(clientSocket, &info, sizeof(info));

    if (numbytes > 0) {
        printf("File sending.. file name: %s, file size:  %ld \n", fname, numbytes);
        write(clientSocket, buffer, numbytes);
    }

    fclose(file);
    free(buffer);
}

static void parse_client_arguments(int argc, char *argv[], struct clientOptions *opts)
{
    int c;
    bool is_serverip_set = false;
    while ((c = getopt(argc, argv, "s:p:*:")) != -1) {
        switch(c) {
            case 's':
            {
                opts->server_ip = optarg;
                is_serverip_set = true;
                break;
            }
            case 'p':
            {
                opts->port = parse_port(optarg, 10);
                break;
            }
            case ':':
            {
                error_message(__FILE__, __func__ , __LINE__, "\"Option requires an operand\"", 5);
                break;
            }
            case '?':
            {
                error_message(__FILE__, __func__ , __LINE__, "Unknown", 6);
                break;
            }
        }
    }

    if (!is_serverip_set)
        error_message(__FILE__, __func__ , __LINE__, "\"Server IP must be included\"", 5);

    int count = 0;
    //Read only txt file
    for (; optind < argc; optind++) {
        if (strstr(argv[optind], ".txt")) {
            files[count++] = argv[optind];
        } else {
            error_message(__FILE__, __func__ , __LINE__, "invalid file name", 6);
        }
    }

    if (count == 0)
        error_message(__FILE__, __func__ , __LINE__, "\"At least one file required\"", 5);

    opts->file_cnt = count;
}
