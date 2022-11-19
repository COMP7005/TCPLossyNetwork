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

typedef struct tcp_info {
    int seq;
    int ack;
    int fin;
} tcp_info;

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

static void parse_proxy_arguments(int argc, char *argv[], struct proxyOptions *opts);
static void options_init(struct proxyOptions *opts);
static void connect_receiver(struct proxyOptions *opts);
static int read_data(int targetSocket, struct proxyOptions *opts, char buffer[SIZE]);

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


static void connect_receiver(struct proxyOptions *opts)
{
    char buffer[SIZE];
    int bytes = 0;

    // Connect to main server(receiver) via this proxy server
    int receiver_fd =0;
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
        //receive data from client
        memset(&buffer, '\0', sizeof(buffer));

        printf("[+]reading data from sender \n");
        bytes = read_data(opts->sender_fd, opts, buffer);

        printf("[+]sending data to receiver \n");
        if (bytes <= 0) {
            printf("[-]sender data cannot be read\n");
        }
        else
        {
             // Send data to main server
              printf("Sending...");
              write(receiver_fd, buffer, sizeof(buffer));
              printf("%s\n", buffer);
              printf("sender fd is : %d\n",opts->sender_fd);
              printf("From client :\n");

//            fputs(buffer,stdout);
//            fflush(stdout);
        }

        printf("[+]reading data to receiver \n");

        printf("[+]sending data to sender \n");

        //receiver response from server
        memset(&buffer, '\0', sizeof(buffer));
        bytes = read(receiver_fd, buffer, sizeof(buffer));
        if (bytes <= 0) {
        } else {
            // send response back to client
//            write(opts->sender_fd, buffer, sizeof(buffer));
//            printf("From server :\n");
//            fputs(buffer,stdout);
        }
    }
}

static int read_data_send_to_receiver
(
    int senderSocket,
    int receiverSocket,
    struct proxyOptions *opts,
    char buffer[SIZE])
{
    int bytes = 0;
    tcp_info tcpInfo;
    bytes = read(senderSocket, &tcpInfo, sizeof(tcp_info));
    if (bytes <= 0)
    {
        printf("[-]cannot read Data\n");
    }
    write(receiverSocket, buffer, WINDOW_SIZE+1);


    //received date
    bytes =  read(targetSocket, buffer, WINDOW_SIZE);
    printf("read Data: %s\n", buffer);
    return bytes;
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

//
//static int send_data(int targetSocket, struct proxyOptions *opts) {
//    // Send TCP info
//    tcp_info tcpInfo;
//    tcpInfo.ack = 1;
//    tcpInfo.seq = 1;
//    tcpInfo.fin = 0;
//
//    write(targetSocket, &tcpInfo, sizeof(tcpInfo));
//
//    buffer = (char*)calloc(numbytes, sizeof(char));
//
//    if (buffer == NULL)
//        EXIT_FAILURE;
//
//    int fileSendingTotalCount = numbytes/WINDOW_SIZE;
//
//    int sentCnt = 0;
//    while (sentCnt <= fileSendingTotalCount){
//        ++sentCnt;
//
//        //read file
//        fread(buffer, sizeof *buffer, WINDOW_SIZE, file);
//
//        printf("Sending....");
//        //write to receiver
//        write(receiverSocket, buffer, WINDOW_SIZE+1);
//        printf("%s\n", buffer);
//
//        check_ack_respond(receiverSocket);
//        write(receiverSocket, &tcpInfo, sizeof(tcpInfo));
//
//        printf("-----------\n");
//    }
//    return EXIT_SUCCESS;
//}

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
