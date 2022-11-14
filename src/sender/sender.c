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

struct senderOptions
{
    char *proxy_ip;
    char *receiver_ip;
    char *sending_ip;
    in_port_t port;
    int file_cnt;
};

static void send_file(FILE *fp, char *fname, int sockfd);
static void options_init(struct senderOptions *opts);
static void parse_sender_arguments(int argc, char *argv[], struct senderOptions *opts);

#define DEFAULT_PORT 5000
#define SIZE 1024
#define TCP_SIZE 6
const char* files[20];

int main (int argc, char *argv[]) {
    struct senderOptions opts;

    options_init(&opts);
    parse_sender_arguments(argc, argv, &opts);

    int receiverSocket, ret;
    struct sockaddr_in serverAddr;

    FILE *fp;

    receiverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiverSocket < 0) {
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);
    }
    printf("[+]Client socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(opts.port);
    serverAddr.sin_addr.s_addr = inet_addr(opts.sending_ip);

    ret = connect(receiverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
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
        send_file(fp, fname, receiverSocket);
    }

    // Indicates end of files.
//    file_info info;
//    strcpy(info.fname, "");
//    info.fsize = 0;
//    write(receiverSocket, &info, sizeof(info));

    close(receiverSocket);
    printf("[+]sent data successfully.\n");

    return EXIT_SUCCESS;
}

static void options_init(struct senderOptions *opts)
{
    memset(opts, 0, sizeof(struct senderOptions));
    opts->proxy_ip = "127.0.0.1"; //default localhost
    opts->receiver_ip = "127.0.0.1"; //default localhost
    opts->sending_ip = "127.0.0.1"; //default localhost
    opts->port  = DEFAULT_PORT;
}

void send_file(FILE *file, char *fname, int receiverSocket) {
    char* buffer;
    long numbytes;

    fseek(file, 0L, SEEK_END);
    numbytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    file_info info;
    strcpy(info.fname, fname);
    info.fsize = numbytes;

//    write(receiverSocket, &info, sizeof(info));

    buffer = (char*)calloc(numbytes, sizeof(char));

    if (buffer == NULL)
        EXIT_FAILURE;

    if (file) {
        //send data
        while (fread(buffer, sizeof *buffer, TCP_SIZE, file) == TCP_SIZE) {
            printf("sending.. %s\n", buffer);
            write(receiverSocket, buffer, TCP_SIZE+1);
        }

        fclose(file);
    }

    //Read response from server
    char response[1024];
    memset(response, 0, sizeof(response));
    if (read(receiverSocket, response, sizeof(response)) < 0) {
        error_message(__FILE__, __func__ , __LINE__, "Cannot read", 7);
    }
    printf("Received: %s\n", response);

    free(buffer);
}

static void parse_sender_arguments(int argc, char *argv[], struct senderOptions *opts)
{
    int c;
    while ((c = getopt(argc, argv, "x:s:r:p:*:")) != -1) {
        switch(c) {
            case 'x':
            {
                opts->proxy_ip = optarg;
                break;
            }
            case 'r':
            {
                opts->receiver_ip = optarg;
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

    //check one of them are set
    //If not return error
    if (!opts->proxy_ip && !opts->receiver_ip)
        error_message(__FILE__, __func__ , __LINE__,
                      "Proxy IP and Receiver IP both has not been set", 6);

    //set proxy ip as sending ip
    if (opts->proxy_ip)
        opts->sending_ip = opts->proxy_ip;
    else
        opts->sending_ip = opts->receiver_ip;

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

