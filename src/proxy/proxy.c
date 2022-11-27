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
#include <stdbool.h>
#include <pthread.h>

#define FILENAME "proxy_info.csv"

struct dataRecord
{
    int sentCnt;
    int recCnt;
    int sentDropCnt;
    int recDropCnt;
};

struct proxyOptions
{
    int drop_data_percent;
    int drop_ack_percent;
    int sender_fd;
    char *receiver_ip;
    in_port_t receiver_port;
    in_port_t proxy_port;
};

#define RECEIVER_DEFAULT_PORT 4444
#define PROXY_DEFAULT_PORT 5000
#define SIZE 1024

static void options_init(struct proxyOptions *opts, struct dataRecord *record);
static void parse_proxy_arguments(int argc, char *argv[], struct proxyOptions *opts);
static void connect_receiver(struct proxyOptions *opts, struct dataRecord *record);
static void transfer_data( int senderSocket, int receiverSocket, struct proxyOptions *opts, struct dataRecord *record);
static int drop_packet(int drop_rate);
static void reset_record(struct dataRecord *record);

int main (int argc, char *argv[])
{
    struct proxyOptions opts;
    struct dataRecord record;
    pid_t childip;

    srand (time(NULL));
    options_init(&opts, &record);
    parse_proxy_arguments(argc, argv, &opts);

    printf("server IP : %s and port %hu " , opts.receiver_ip, opts.receiver_port);
    printf("proxy port is %hu", opts.proxy_port);
    printf("\n");

    // Socket variables
    int proxy_fd = 0, sender_fd= 0;
    struct sockaddr_in proxy_sd;
    struct sockaddr_in clientAddr;
    socklen_t addr_size;

    // Add this line only if server exits when client exits
    //signal(SIGPIPE,SIG_IGN);

    // create a socket
    if ((proxy_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error_errno(__FILE__, __func__ , __LINE__, errno, 1);

    printf("Proxy created\n");
    memset(&proxy_sd, 0, sizeof(proxy_sd));

    // Set proxy socket variables
    proxy_sd.sin_family = AF_INET;
    proxy_sd.sin_port = htons(opts.proxy_port);
    proxy_sd.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket
    if ((bind(proxy_fd, (struct sockaddr*)&proxy_sd,sizeof(proxy_sd))) < 0)
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);

    // Start listening to the port for new connections
    if ((listen(proxy_fd, SOMAXCONN)) < 0)
        error_errno(__FILE__, __func__ , __LINE__, errno, 3);

    printf("[+]waiting for connection..\n");

    // Accept one client at one time
    while(1)
    {
        sender_fd = accept(proxy_fd, (struct sockaddr*)&clientAddr, &addr_size);
        char * clientip = inet_ntoa(clientAddr.sin_addr);

        if (sender_fd < 0)
            error_errno(__FILE__, __func__ , __LINE__, errno, 4);

        printf("[+]client %s connected\n", clientip);
        write_stat_header(FILENAME, "Time,Sent,Received,Sent_Drop,Received_Drop");
        reset_record(&record);

        if (sender_fd > 0)
        {
            opts.sender_fd = sender_fd;
            connect_receiver((void *)&opts, (void *)&record);
            sleep(1);
        }
    }

    close(sender_fd);
    close(proxy_fd);
    return EXIT_SUCCESS;
}

static int drop_packet(int drop_rate)
{
    int rand_num;
    rand_num = rand() % 99;
    // printf("drop_rate: %d | rand_num: %d - ", drop_rate, rand_num);

    if (rand_num >= drop_rate)
    {
        // printf("send\n");
        return 1; // fail
    }
    else
    {
        // printf("drop\n");
        return 0; // success
    }
}

static void connect_receiver(struct proxyOptions *opts, struct dataRecord *record)
{
    char buffer[SIZE];
    int bytes = 0;

    // Connect to main server(receiver) via this proxy server
    int receiver_fd = 0;
    struct sockaddr_in receiver_sd;

    // Create a socket
    receiver_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (receiver_fd < 0)
        printf("[-]server socket not created\n");

    printf("[+]server socket created\n");
    memset(&receiver_sd, 0, sizeof(receiver_sd));

    // Set socket variables
    receiver_sd.sin_family = AF_INET;
    receiver_sd.sin_port = htons(opts->receiver_port);
    receiver_sd.sin_addr.s_addr = inet_addr(opts->receiver_ip);

    // Connect to main server from this proxy server
    if ((connect(receiver_fd, (struct sockaddr *)&receiver_sd, sizeof(receiver_sd))) < 0)
        printf("[-]receiver connection not established\n");

    printf("[+]server socket connected\n");
    transfer_data(opts->sender_fd, receiver_fd, opts, record);
}

static void transfer_data(int senderSocket, int receiverSocket,
                            struct proxyOptions *opts, struct dataRecord *record)
{
    int bytes1 = 0, bytes2 = 0;
    struct tcpInfo senderInfo;
    struct tcpInfo receiverInfo;

    while(1)
    {
        bytes1 = read(senderSocket, &senderInfo, sizeof(senderInfo));

        if (bytes1 <= 0)
            printf("[-]cannot read Data\n");

        printf("\n[+]Proxy received a packet from \"Sender\": %s\n", senderInfo.data);

        write_stat_proxy(FILENAME, record->sentCnt, ++record->recCnt, \
                            record->sentDropCnt, record->recDropCnt, (void *)&senderInfo);

        // Don't drop the finish
        if (senderInfo.fin != 1 && drop_packet(opts->drop_data_percent) == 0) // when the packet is dropped
        {
            printf("[!]The packet from sender is dropped\n");
            ++record->sentDropCnt;
            continue;
        }

        write(receiverSocket, &senderInfo, sizeof(senderInfo));
        printf("[+]Proxy sent a packet to \"Receiver\": %s\n", senderInfo.data);

        write_stat_proxy(FILENAME, ++record->sentCnt, record->recCnt, \
                            record->sentDropCnt, record->recDropCnt, (void *)&senderInfo);

        bytes2 = read(receiverSocket, &receiverInfo, sizeof(receiverInfo));
        printf("\n[+]Proxy received a packet from \"Receiver\": %s\n", receiverInfo.data);

        write_stat_proxy(FILENAME, record->sentCnt, ++record->recCnt, \
                            record->sentDropCnt, record->recDropCnt, (void *)&receiverInfo);

        // Don't drop the finish
        if (receiverInfo.fin != 1 && drop_packet(opts->drop_ack_percent) == 0) // when the packet is dropped
        {
            printf("[!]The packet from receiver is dropped\n");
            ++record->recDropCnt;
            continue;
        }

        write(senderSocket, &receiverInfo, sizeof(receiverInfo));
        printf("[+]Proxy sent a packet to \"Sender\": %s\n", receiverInfo.data);

        write_stat_proxy(FILENAME, ++record->sentCnt, record->recCnt, \
                            record->sentDropCnt, record->recDropCnt, (void *)&receiverInfo);

        if (receiverInfo.fin == 1)
        {
            printf("[*]Please check the statistics (file: %s)\n", FILENAME);
            printf("@@@@@@@@@@@@@@@@@@@@ Data Transfer Completed @@@@@@@@@@@@@@@@@@@@\n");
            break;
        }
    }
    close(receiverSocket);
}

static void parse_proxy_arguments(int argc, char *argv[], struct proxyOptions *opts)
{
    int c;
    bool is_serverip_set = false;
    while ((c = getopt(argc, argv, "r:i:o:d:a:")) != -1)
    {
        switch(c)
        {
            case 'r':
            {
                opts->receiver_ip = optarg;
                is_serverip_set = true;
                break;
            }
            case 'i': //port in
            {
                // This is server port => proxy will connect to server with this port
                opts->receiver_port = parse_port(optarg, 10);
                break;
            }
            case 'o': //port out
            {
                // This is proxt port => sender will connect with this port
                opts->proxy_port = parse_port(optarg, 10);
                break;
            }
            case 'd': //drop data percentage
            {
                int data_drop_rate = atoi(optarg);
                if (data_drop_rate < 0 || data_drop_rate > 100)
                {
                    error_message(__FILE__, __func__, __LINE__,
                                  "\"Drop rate is between 0 and 100\"", 5);
                }
                opts->drop_data_percent = data_drop_rate;
                break;
            }
            case 'a': //drop ack percentage
            {
                int ack_drop_rate = atoi(optarg);
                if (ack_drop_rate < 0 || ack_drop_rate > 100)
                {
                    error_message(__FILE__, __func__, __LINE__,
                                  "\"Drop rate is between 0 and 100\"", 5);
                }
                opts->drop_ack_percent = ack_drop_rate;
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
}

static void options_init(struct proxyOptions *opts, struct dataRecord *record)
{
    memset(opts, 0, sizeof(struct proxyOptions));
    memset(record, 0, sizeof(struct dataRecord));
    opts->receiver_ip = "127.0.0.1"; //default localhost
    opts->proxy_port = PROXY_DEFAULT_PORT;
    opts->receiver_port = RECEIVER_DEFAULT_PORT;
    opts->drop_ack_percent = 0;
    opts->drop_data_percent = 0;
    record->sentCnt = 0;
    record->recCnt = 0;
    record->sentDropCnt = 0;
    record->recDropCnt = 0;
}

static void reset_record(struct dataRecord *record)
{
    record->sentCnt = 0;
    record->recCnt = 0;
    record->sentDropCnt = 0;
    record->recDropCnt = 0;
}
