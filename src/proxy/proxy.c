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

struct proxyOptions
{
    int drop_data_per;
    int drop_ack_per;
    char *sender_ip;
    char *receiver_ip;
    in_port_t port_in;
    in_port_t port_out;

    int sender_fd;
};


static void parse_proxy_arguments(int argc, char *argv[], struct proxyOptions *opts);
static void options_init(struct proxyOptions *opts);
static void connect_receiver(struct proxyOptions *opts);

#define DEFAULT_PORT 5000

int main (int argc, char *argv[]) {
    struct proxyOptions opts;

    options_init(&opts);
    parse_proxy_arguments(argc, argv, &opts);

    pthread_t tid;

    printf("server IP : %s and port %hu" , opts.receiver_ip, opts.port_in);
    printf("proxy port is %hu",opts.port_out);
    printf("\n");

    //socket variables
    int proxy_fd =0, client_fd=0;
    struct sockaddr_in proxy_sd;
    // add this line only if server exits when client exits
    signal(SIGPIPE,SIG_IGN);
    // create a socket
    if((proxy_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nFailed to create socket");
    }
    printf("Proxy created\n");
    memset(&proxy_sd, 0, sizeof(proxy_sd));

    // set socket variables
    proxy_sd.sin_family = AF_INET;
    proxy_sd.sin_port = htons(opts.port_in);
    proxy_sd.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket
    if((bind(proxy_fd, (struct sockaddr*)&proxy_sd,sizeof(proxy_sd))) < 0)
    {
        printf("[-]Failed to bind a socket");
    }

    // start listening to the port for new connections
    if((listen(proxy_fd, SOMAXCONN)) < 0)
    {
        printf("[-]Failed to listen");
    }
    printf("[+]waiting for connection..\n");

    //accept all client connections continuously
    while(1)
    {
        client_fd = accept(proxy_fd, (struct sockaddr*)NULL ,NULL);
        printf("client no. %d connected\n",client_fd);
        if (client_fd > 0)
        {
            //multithreading variables
            opts.sender_fd = client_fd;
            pthread_create(&tid, NULL, connect_receiver, (void *)&opts);
            sleep(1);
        }
    }

    return EXIT_SUCCESS;
}

static void connect_receiver(struct proxyOptions *opts)
{
    char buffer[65535];
    int bytes =0;

    //code to connect to main server via this proxy server
    int receiver_fd =0;
    struct sockaddr_in receiver_sd;
    // create a socket
    receiver_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(receiver_fd < 0)
    {
        printf("server socket not created\n");
    }
    printf("server socket created\n");
    memset(&receiver_sd, 0, sizeof(receiver_sd));
    // set socket variables
    receiver_sd.sin_family = AF_INET;
    receiver_sd.sin_port = htons(opts->port_out);
    receiver_sd.sin_addr.s_addr = inet_addr(opts->receiver_ip);


    //connect to main server from this proxy server
    if((connect(receiver_fd, (struct sockaddr *)&receiver_sd, sizeof(receiver_sd)))<0)
    {
        printf("[-]receiver connection not established");
    }
    printf("[+]server socket connected\n");
    while(1)
    {
        //receive data from client
        memset(&buffer, '\0', sizeof(buffer));
        bytes = read(opts->sender_fd, buffer, sizeof(buffer));
        if(bytes <= 0)
        {
        }
        else
        {
            // send data to main server
            write(receiver_fd, buffer, sizeof(buffer));
            //printf("client fd is : %d\n",c_fd);
            printf("From client :\n");
            fputs(buffer,stdout);
            fflush(stdout);
        }

        //recieve response from server
        memset(&buffer, '\0', sizeof(buffer));
        bytes = read(receiver_fd, buffer, sizeof(buffer));
        if(bytes <= 0)
        {
        }
        else
        {
            // send response back to client
            write(opts->sender_fd, buffer, sizeof(buffer));
            printf("From server :\n");
            fputs(buffer,stdout);
        }
    };
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
                opts->port_in = parse_port(optarg, 10);
                break;
            }
            case 'o': //port out
            {
                opts->port_out = parse_port(optarg, 10);
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
    opts->sender_ip = "127.0.0.1"; //default localhost
    opts->port_in  = DEFAULT_PORT;
    opts->port_out  = DEFAULT_PORT;
}
