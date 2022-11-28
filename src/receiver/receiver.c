#include "../conversion.c"
#include "../utils.c"
#include "../error.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define FILENAME "receiver_info.csv"

struct dataRecord
{
    int sentCnt;
    int recCnt;
};

struct receiverOptions
{
    char *ip_in;
    in_port_t port_in;
};

static void options_init(struct receiverOptions *opts, struct dataRecord *record);
static void parse_receiver_arguments(int argc, char *argv[], struct receiverOptions *opts);
static int read_data(int newSocket, struct receiverOptions *opts, struct dataRecord *record);

#define SIZE 1024
#define DEFAULT_RECEIVER_PORT 4444
#define MAX_PENDING 10

int main (int argc, char *argv[])
{
    struct receiverOptions opts;
    struct dataRecord record;

    options_init(&opts, &record);
    parse_receiver_arguments(argc, argv, &opts);

    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    pid_t childip;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error_errno(__FILE__, __func__ , __LINE__, errno, 1);

    printf("[+]Server socket created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(opts.port_in);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind ip address to specific port
    ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);

    if (listen(sockfd, MAX_PENDING) == 0)
        printf("Listening...\n");
    else
        error_errno(__FILE__, __func__ , __LINE__, errno, 3);

    while (1)
    {
        write_stat_header(FILENAME, "Time,Sent,Received");
        newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);

        if (newSocket < 0)
            error_errno(__FILE__, __func__ , __LINE__, errno, 4);

        //set current input ip
        opts.ip_in = inet_ntoa(newAddr.sin_addr);

        printf("\n[+]Connection accept from %s:%d\n", opts.ip_in, opts.port_in);

        if ((childip = fork()) == 0)
        {
            close(sockfd);

            if (read_data(newSocket, &opts, &record) == 0)
            {
                printf("[+]Finished.\n");
                printf("[*]Please check the statistics (file: %s)\n", FILENAME);
                break;
            }
        }
    }

    close(newSocket);
    close(sockfd);
    return EXIT_SUCCESS;
}


static void parse_receiver_arguments(int argc, char *argv[], struct receiverOptions *opts)
{
    int c;
    while ((c = getopt(argc, argv, "p:")) != -1)
    {
        switch(c)
        {
            case 'p':
            {
                opts->port_in = parse_port(optarg, 10);
                break;
            }
            case ':':
            {
                error_message(__FILE__, __func__ , __LINE__, "\"Option requires an operand\"", 5);
            }
            case '?':
            {
                error_message(__FILE__, __func__ , __LINE__, "Unknown", 6);
            }
        }
    }

    if (!opts->port_in)
        error_message(__FILE__, __func__ , __LINE__,
                      "Receiver port needs to be set", 7);

    printf("[+]Port: %hu\n", opts->port_in);
}

static int read_data(int newSocket, struct receiverOptions *opts, struct dataRecord *record)
{
    int i = 0;
    while (1)
    {
        struct tcpInfo tcpInfo;
        struct tcpInfo tcpInfo2;

        read(newSocket, &tcpInfo, sizeof(tcpInfo));

        if (tcpInfo.fin == 1) {
            tcpInfo2.ack = tcpInfo.seq;
            tcpInfo2.seq = 1;
            tcpInfo2.fin = 1;
            strcpy(tcpInfo2.data, "ACK");

            // write_to_file_tmp(FILENAME, tcpInfo2.data, record->sentCnt++);
            // write_stat(FILENAME, tcpInfo2.data, record->pshCnt++, record->ackCnt);

            //send ACK
            write(newSocket, &tcpInfo2, sizeof(tcpInfo2));
            break;
        }

        printf("\n[Received]: %s\n", tcpInfo.data);
        write_stat(FILENAME, tcpInfo.data, record->sentCnt, ++record->recCnt);

        //Sending ACK
        tcpInfo2.ack = tcpInfo2.seq;
        tcpInfo2.seq = 1;
        tcpInfo2.fin = 0;
        strcpy(tcpInfo2.data, "ACK");

        //send ACK
        write(newSocket, &tcpInfo2, sizeof(tcpInfo2));
        printf("[Sending]: %s\n", tcpInfo2.data);
        write_stat(FILENAME, tcpInfo2.data, ++record->sentCnt, record->recCnt);
        i++;
    }
    return EXIT_SUCCESS;
}

static void options_init(struct receiverOptions *opts, struct dataRecord *record)
{
    memset(opts, 0, sizeof(struct receiverOptions));
    memset(record, 0, sizeof(struct dataRecord));
//    opts->port_in  = DEFAULT_RECEIVER_PORT;
    record->sentCnt = 0;
    record->recCnt = 0;
}