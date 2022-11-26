#include "../conversion.c"
#include "../error.c"
#include "../utils.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define DEFAULT_PROXY_PORT 5000
#define SIZE 1024
#define WINDOW_SIZE 6
#define TCP_TIME_OUT 3
#define FILENAME "sender_info.csv"

struct dataRecord {
    int sentCnt;
    int recCnt;
};

struct senderOptions
{
    char *proxy_ip;
    char *receiver_ip;
    char *sending_ip;
    in_port_t port;
    int file_cnt;
};

static void options_init(struct senderOptions *opts, struct dataRecord *record);
static void parse_sender_arguments(int argc, char *argv[], struct senderOptions *opts);
static void send_file(FILE *fp, char *fname, int sockfd, struct  dataRecord *record);
static void check_ack_respond(int receiverSocket, struct tcpInfo *tcpSend, struct dataRecord *record);
static void send_fin_data(int receiverSocket, struct tcpInfo tcpSend, int seqCnt);

const char* files[20];

int main (int argc, char *argv[]) {
    struct senderOptions opts;
    struct dataRecord record;
    struct timeval tv;

    options_init(&opts, &record);
    parse_sender_arguments(argc, argv, &opts);

    int receiverSocket, ret;
    struct sockaddr_in serverAddr;

    tv.tv_sec = TCP_TIME_OUT;
    tv.tv_usec = 0;

    FILE *fp;
    FILE *sender_fp;

    receiverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiverSocket < 0) {
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);
    }

    //set timer for receiver method.
    setsockopt(receiverSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
    printf("[+]Client socket is created.\n");
    write_stat_header(FILENAME, "Time,Sent,Received");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(opts.port);
    serverAddr.sin_addr.s_addr = inet_addr(opts.sending_ip);

    ret = connect(receiverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret < 0) {
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);
    }
    printf("[+]Connect to Server.\n");
//    sender_fp = fopen(FILENAME, "a");
//    fclose(sender_fp);

    for (int i = 0; i < opts.file_cnt; i++){
        fp = fopen(files[i], "r");
        if (fp == NULL) {
            error_message(__FILE__, __func__ , __LINE__, "Can't read a file", 2);
        }
        char *fname = (char*) malloc(sizeof(files[i])+1);
        strcpy(fname, files[i]);

        printf("[+]Sending: %s....\n\n", fname);
        send_file(fp, fname, receiverSocket, &record);
    }

//    fclose(sender_fp);
    close(receiverSocket);
    printf("[+]sent data successfully.\n");

    return EXIT_SUCCESS;
}

static void send_file(FILE *file, char *fname,
                        int receiverSocket, struct  dataRecord *record)
{
    char* buffer;
    long numbytes;
    int seqCnt = 0;

    if (!file)
        error_message(__FILE__, __func__ , __LINE__, "Cannot read the file", 7);

    fseek(file, 0L, SEEK_END);
    numbytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    buffer = (char*)calloc(numbytes, sizeof(char));

    if (buffer == NULL)
        EXIT_FAILURE;

    int fileSendingTotalCount = numbytes/WINDOW_SIZE;
    int remain = numbytes%WINDOW_SIZE;
    if (remain != 0)
        fileSendingTotalCount++;
    int sentCnt = 0;

    struct tcpInfo tcpSend;
    while (sentCnt < fileSendingTotalCount){
        ++sentCnt;

        //read file
        fread(buffer, sizeof *buffer, WINDOW_SIZE, file);

        printf("[Sending]: ");
        tcpSend.ack = 1;
        tcpSend.seq = seqCnt;
        tcpSend.fin = 0;
        strcpy(tcpSend.data, buffer);
        printf("%s\n", tcpSend.data);

        write(receiverSocket, &tcpSend, sizeof(tcpSend));
        write_stat(FILENAME, tcpSend.data, ++record->sentCnt, record->recCnt);

        seqCnt = seqCnt + WINDOW_SIZE;
        check_ack_respond(receiverSocket, &tcpSend, record);
        printf("\n-----------\n\n");
    }
    printf("[+]Sending: fin\n");
    send_fin_data(receiverSocket, tcpSend, seqCnt);

    fclose(file);
    free(buffer);
}


static void check_ack_respond(int receiverSocket, struct tcpInfo *tcpSend, struct dataRecord *record)
{
    while(1) {
        int bytes = 0;
        struct tcpInfo tcpReceive;

        bytes = read(receiverSocket, &tcpReceive, sizeof(tcpReceive));

        if (bytes > 0 )
        {
            printf("[received]: %s\n", tcpReceive.data);
            write_stat(FILENAME, tcpReceive.data, record->sentCnt, ++record->recCnt);
            break;
        }
        else // resend
        {
            printf("[Resending]: %s\n", tcpSend->data);
            write(receiverSocket, &tcpSend, sizeof(tcpSend));
            write_stat(FILENAME, tcpSend->data, ++record->sentCnt, record->recCnt);
        }
    }
}

static void send_fin_data(int receiverSocket, struct tcpInfo tcpSend, int seqCnt)
{
    //send finish data
    tcpSend.ack = 1;
    tcpSend.seq = seqCnt;
    tcpSend.fin = 1;
    strcpy(tcpSend.data, "FIN");

    write(receiverSocket, &tcpSend, sizeof(tcpSend));
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

static void options_init(struct senderOptions *opts, struct dataRecord *record)
{
    memset(opts, 0, sizeof(struct senderOptions));

    opts->proxy_ip = "127.0.0.1"; //default localhost
    opts->receiver_ip = "127.0.0.1"; //default localhost
    opts->sending_ip = "127.0.0.1"; //default localhost
    opts->port  = DEFAULT_PROXY_PORT;

    memset(record, 0, sizeof(struct dataRecord));
    record->sentCnt = 0;
    record->recCnt = 0;
}
