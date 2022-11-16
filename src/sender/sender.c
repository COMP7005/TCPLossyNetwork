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

typedef struct tcp_info {
    int seq;
    int ack;
    int fin;
} tcp_info;

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
static int check_ack_respond(int receiverSocket);

#define DEFAULT_PORT 5000
#define SIZE 1024
#define WINDOW_SIZE 6
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

    if (!file)
        error_message(__FILE__, __func__ , __LINE__, "Cannot read the file", 7);

    fseek(file, 0L, SEEK_END);
    numbytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    tcp_info tcpInfo;
    tcpInfo.ack = 1;
    tcpInfo.seq = 1;
    tcpInfo.fin = 0;

    write(receiverSocket, &tcpInfo, sizeof(tcpInfo));

    buffer = (char*)calloc(numbytes, sizeof(char));

    if (buffer == NULL)
        EXIT_FAILURE;

    int fileSendingTotalCount = numbytes/WINDOW_SIZE;

    int sentCnt = 0;
    while (sentCnt <= fileSendingTotalCount){
        ++sentCnt;

        //read file
        fread(buffer, sizeof *buffer, WINDOW_SIZE, file);

        printf("Sending....");
        //write to receiver
        write(receiverSocket, buffer, WINDOW_SIZE+1);
        printf("%s\n", buffer);

        check_ack_respond(receiverSocket);
        write(receiverSocket, &tcpInfo, sizeof(tcpInfo));

        printf("-----------\n");
    }

    //let receiver know finished.
    tcpInfo.ack = 1;
    tcpInfo.seq = 1;
    tcpInfo.fin = 1;
    write(receiverSocket, &tcpInfo, sizeof(tcpInfo));

    fclose(file);
    free(buffer);
}

static int check_ack_respond(int receiverSocket){
    char response[10];
    memset(response, 0, sizeof(response));

    if (read(receiverSocket, response, sizeof(response)) < 0){
        printf("-------error\n");
    }

    printf("received %s\n", response);
//    if (strstr(response, "ACK"))
//    {
//        return EXIT_SUCCESS;
//    }
//    else
//    {
//        return EXIT_FAILURE;
//    }
    return 1;
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

