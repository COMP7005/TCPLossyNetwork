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

typedef struct fileInfo {
    char fname[NAME_MAX + 1];
    long fsize;
} file_info;

struct receiverOptions
{
    char *ip_in;
    in_port_t port_in;
};

static void options_init(struct receiverOptions *opts);
static void parse_receiver_arguments(int argc, char *argv[], struct receiverOptions *opts);
static int read_file(int newSocket, struct receiverOptions *opts);

#define TCP_SIZE 20
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

        if (read_file(newSocket, &opts) == 0) {
            char buf[5] = "ACK";
            write(newSocket, buf, TCP_SIZE+1);
        }

        close(newSocket);
    }

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

static int read_file(int newSocket, struct receiverOptions *opts) {

    char buffer[SIZE];
    //Get the content
    read(newSocket, buffer, TCP_SIZE+1);

    printf("%s \n",  buffer);

    return EXIT_SUCCESS;
}

static void options_init(struct receiverOptions *opts) {
    memset(opts, 0, sizeof(struct receiverOptions));
    opts->port_in  = DEFAULT_PORT;
}