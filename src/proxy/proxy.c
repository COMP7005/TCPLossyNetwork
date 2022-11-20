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
#include <pthread.h>
#include <netdb.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

struct tcpInfo {
    int seq;
    int ack;
    int fin;
    char data[NAME_MAX];
};

struct proxyOptions
{
    int drop_data_per;
    int drop_ack_per;
    char *receiver_ip;
    in_port_t server_port;
    in_port_t proxy_port;

    int sender_fd;
};

#define DEFAULT_PORT 5000
#define WINDOW_SIZE 20
#define SIZE 1024

static void options_init(struct proxyOptions *opts);
static void parse_proxy_arguments(int argc, char *argv[], struct proxyOptions *opts);
static void connect_receiver(struct proxyOptions *opts);
static void transfer_data( int senderSocket, int receiverSocket, struct proxyOptions *opts);
static int drop_packet(int drop_rate)

int main (int argc, char *argv[]) {
    struct proxyOptions opts;

    options_init(&opts);
    parse_proxy_arguments(argc, argv, &opts);

    printf("server IP : %s and port %hu " , opts.receiver_ip, opts.server_port);
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
    {
        printf("[-]Failed to create socket\n");
    }
    printf("Proxy created\n");
    memset(&proxy_sd, 0, sizeof(proxy_sd));

    // Set proxy socket variables
    proxy_sd.sin_family = AF_INET;
    proxy_sd.sin_port = htons(opts.proxy_port);
    proxy_sd.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket
    if ((bind(proxy_fd, (struct sockaddr*)&proxy_sd,sizeof(proxy_sd))) < 0)
    {
        printf("[-]Failed to bind a socket\n");
    }

    // Start listening to the port for new connections
    if ((listen(proxy_fd, SOMAXCONN)) < 0)
    {
        printf("[-]Failed to listen\n");
    }
    printf("[+]waiting for connection..\n");

    // Accept one client at one time
    while(1)
    {
        sender_fd = accept(proxy_fd, (struct sockaddr*)&clientAddr, &addr_size);
        char * clientip = inet_ntoa(clientAddr.sin_addr);

        printf("client no. %d %s connected\n", sender_fd, clientip);

        if (sender_fd > 0)
        {
            opts.sender_fd = sender_fd;
            connect_receiver((void *)&opts);
            sleep(1);
        }
    }

    close(sender_fd);
    close(proxy_fd);
    return EXIT_SUCCESS;
}

static int drop_packet(int drop_rate)
{
    int drop_rate, rand_num;
    rand_num = rand() % 99;
    printf("drop_rate: %d | rand_num: %d - ", drop_rate, rand_num);

    if (rand_num >= drop_rate)
    {
        printf("send\n");
        return 1 // fail
    }
    else
    {
        printf("drop\n");
        return 0 // success
    }
}

static void connect_receiver(struct proxyOptions *opts)
{
    char buffer[SIZE];
    int bytes = 0;

    // Connect to main server(receiver) via this proxy server
    int receiver_fd = 0;
    struct sockaddr_in receiver_sd;

    // Create a socket
    receiver_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (receiver_fd < 0)
    {
        printf("[-]server socket not created\n");
    }

    printf("server socket created\n");
    memset(&receiver_sd, 0, sizeof(receiver_sd));

    // Set socket variables
    receiver_sd.sin_family = AF_INET;
    receiver_sd.sin_port = htons(opts->server_port);
    receiver_sd.sin_addr.s_addr = inet_addr(opts->receiver_ip);

    // Connect to main server from this proxy server
    if ((connect(receiver_fd, (struct sockaddr *)&receiver_sd, sizeof(receiver_sd))) < 0)
    {
        printf("[-]receiver connection not established\n");
    }

    printf("[+]server socket connected\n");
    while(1) {
        transfer_data(opts->sender_fd, receiver_fd, opts);
    }
}

static void transfer_data
(
    int senderSocket,
    int receiverSocket,
    struct proxyOptions *opts)
{
    int bytes1 = 0, bytes2 = 0;
    struct tcpInfo tcpInfo;
    bytes1 = read(senderSocket, &tcpInfo, sizeof(tcpInfo));
    if (bytes1 <= 0)
    {
        printf("[-]cannot read Data\n");
    }
    printf("[+]Proxy received from \"Sender\": %s\n", tcpInfo.data);

    write(receiverSocket, &tcpInfo, sizeof(tcpInfo));
    printf("[+]Proxy send to \"Receiver\": %s\n\n", tcpInfo.data);

    bytes2 = read(receiverSocket, &tcpInfo, sizeof(tcpInfo));
    printf("[+]Proxy received from \"Receiver\": %s\n", tcpInfo.data);

    write(senderSocket, &tcpInfo, sizeof(tcpInfo));
    printf("[+]Proxy send to \"Sender\": %s\n\n", tcpInfo.data);

}

static int check_ack_respond(int receiverSocket){
    char response[10];
    memset(response, 0, sizeof(response));

    if (read(receiverSocket, response, sizeof(response)) < 0){
        printf("-------error\n");
    }

    printf("received %s\n", response);
    return EXIT_SUCCESS;
}


static void parse_proxy_arguments(int argc, char *argv[], struct proxyOptions *opts)
{
    int c;
    bool is_serverip_set = false;
    while ((c = getopt(argc, argv, "r:i:o:d:a:")) != -1) {
        switch(c) {
            case 'r':
            {
                opts->receiver_ip = optarg;
                is_serverip_set = true;
                break;
            }
            case 'i': //port in
            {
                // This is server port => proxy will connect to server with this port
                opts->server_port = parse_port(optarg, 10);
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
                opts->drop_data_per = atoi(optarg);
                break;
            }
            case 'a': //drop ack percentage
            {
                opts->drop_ack_per = atoi(optarg);
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


static void options_init(struct proxyOptions *opts)
{
    memset(opts, 0, sizeof(struct proxyOptions));
    opts->receiver_ip = "127.0.0.1"; //default localhost
    opts->proxy_port  = DEFAULT_PORT;
    opts->server_port  = DEFAULT_PORT;
}
