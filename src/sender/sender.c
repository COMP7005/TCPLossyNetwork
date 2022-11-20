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


#define DEFAULT_PORT 5000
#define SIZE 1024
#define WINDOW_SIZE 6

struct dataRecord {
    int pshCnt;
    int ackCnt;
};

struct tcpInfo {
    int seq;
    int ack;
    int fin;
    char data[NAME_MAX + 1];
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
static void send_file(FILE *fp, char *fname, int sockfd, FILE *sender_fp, struct  dataRecord *record);
static void check_ack_respond(int receiverSocket, FILE *sender_fp, struct dataRecord *record);
static void write_to_file(FILE *fp, char* data, int counter);
void sigalrm_handler(int);

const char* files[20];
bool is_ack_receiver = false;

# define T 5

int flag = T;

int main (int argc, char *argv[]) {
    struct senderOptions opts;
    struct dataRecord record;

    options_init(&opts, &record);
    parse_sender_arguments(argc, argv, &opts);

    int receiverSocket, ret;
    struct sockaddr_in serverAddr;

    FILE *fp;
    FILE *sender_fp;

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

        printf("[+]Sending: %s....\n\n", fname);
        send_file(fp, fname, receiverSocket, sender_fp, &record);
    }

    close(receiverSocket);
    printf("[+]sent data successfully.\n");

    return EXIT_SUCCESS;
}

static void write_to_file(FILE *fp, char* data, int counter)
{
    fp = fopen("sender_info.csv", "a");
    fprintf(fp, "[%d]: %s\n", counter, data);
    fclose(fp);
}

void sigalrm_handler(int sig)
{
    if(--flag){
        printf("Hi...\n");   /*version 1*/
        /*printf("Hi...");*/ /*version 2*/
    }else{
        printf("BYE\n");
        flag=T;
    }
    alarm(1);
}

static void send_file(FILE *file, char *fname, int receiverSocket, FILE *sender_fp, struct  dataRecord *record) {
    char* buffer;
    long numbytes;

    if (!file)
        error_message(__FILE__, __func__ , __LINE__, "Cannot read the file", 7);

    fseek(file, 0L, SEEK_END);
    numbytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    buffer = (char*)calloc(numbytes, sizeof(char));

    if (buffer == NULL)
        EXIT_FAILURE;

    int fileSendingTotalCount = numbytes/WINDOW_SIZE;
    int sentCnt = 0;
    int seqCnt = 0;
    struct tcpInfo tcpInfo;
    while (sentCnt <= fileSendingTotalCount){
        ++sentCnt;

        //read file
        fread(buffer, sizeof *buffer, WINDOW_SIZE, file);

        printf("[Sending]: ");
        tcpInfo.ack = 1;
        tcpInfo.seq = seqCnt;
        tcpInfo.fin = 0;

        strcpy(tcpInfo.data, buffer);
        printf("%s\n", tcpInfo.data);

        write(receiverSocket, &tcpInfo, sizeof(tcpInfo));
        write_to_file(sender_fp, buffer, record->pshCnt++);
        seqCnt = seqCnt + WINDOW_SIZE;

        signal(SIGALRM, sigalrm_handler);
        alarm (1);

        while(1) {
            is_ack_receiver = false;
            check_ack_respond(receiverSocket, sender_fp, record);
            if (is_ack_receiver)
                break;
            write(receiverSocket, &tcpInfo, sizeof(tcpInfo));
        }


        printf("\n-----------\n\n");
    }

    //send fin data
    tcpInfo.ack = 1;
    tcpInfo.seq = seqCnt;
    tcpInfo.fin = 1;
    strcpy(tcpInfo.data, "");

    write(receiverSocket, &tcpInfo, sizeof(tcpInfo));

    fclose(file);
    free(buffer);
}


static void check_ack_respond(int receiverSocket, FILE *sender_fp, struct dataRecord *record){
    int bytes = 0;
    struct tcpInfo tcpInfo;

    bytes = read(receiverSocket, &tcpInfo, sizeof(tcpInfo));

    if (bytes > 0 ) {
        is_ack_receiver = true;
        printf("[received]: %s\n", tcpInfo.data);
        write_to_file(sender_fp, tcpInfo.data, record->ackCnt++);
    }
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
    opts->port  = DEFAULT_PORT;

    memset(record, 0, sizeof(struct dataRecord));
    record->ackCnt = 0;
    record->pshCnt = 0;
}
