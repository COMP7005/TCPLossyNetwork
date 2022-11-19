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

typedef struct tcp_info {
    int seq;
    int ack;
    int fin;
} tcp_info;

struct receiverOptions
{
    char *ip_in;
    in_port_t port_in;
};

static void options_init(struct receiverOptions *opts);
static void parse_receiver_arguments(int argc, char *argv[], struct receiverOptions *opts);
static int read_file(int newSocket, struct receiverOptions *opts);

#define WINDOW_SIZE 20
#define SIZE 1024
#define DEFAULT_PORT 5000
#define MAX_PENDING 10


int main (int argc, char *argv[]) {
    struct receiverOptions opts;

    options_init(&opts);
    parse_receiver_arguments(argc, argv, &opts);

    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    pid_t childip;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);
    }
    printf("[+]Server socket created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(opts.port_in);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind ip address to specific port
    ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret < 0) {
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);
    }

    if (listen(sockfd, MAX_PENDING) == 0)
        printf("Listening...\n");
    else
        error_errno(__FILE__, __func__ , __LINE__, errno, 2);

    while (1) {
        newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);

        if (newSocket < 0)
            error_errno(__FILE__, __func__ , __LINE__, errno, 2);

        //set current input ip
        opts.ip_in = inet_ntoa(newAddr.sin_addr);

        printf("[+]Connection accept from %s:%d\n", opts.ip_in, opts.port_in);

        if ((childip = fork()) == 0) {
            close(sockfd);

            if (read_file(newSocket, &opts) == 0) {
                printf("[+]Finished.\n");
                break;
            }
        }
    }

    close(newSocket);
    close(sockfd);
    return EXIT_SUCCESS;
}


static void parse_receiver_arguments(int argc, char *argv[], struct receiverOptions *opts) {
    int c;
    while ((c = getopt(argc, argv, "p:")) != -1) {
        switch(c) {
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
    printf("[+]Port: %hu\n", opts->port_in);
}

static int read_sender_sent(int newSocket, struct receiverOptions *opts) {
    while (1) {
        tcp_info tcpInfo;
        read(newSocket, &tcpInfo, sizeof(tcp_info));

        if (tcpInfo.fin == 1)
            break;

        char buffer[SIZE];
        //received date
        read(newSocket, buffer, WINDOW_SIZE);

        printf("%s \n", buffer);

        //send ACK
        write(newSocket, "ACK", 4);
    }
    return EXIT_SUCCESS;
}

static void options_init(struct receiverOptions *opts) {
    memset(opts, 0, sizeof(struct receiverOptions));
    opts->port_in  = DEFAULT_PORT;
}