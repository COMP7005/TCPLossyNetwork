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

struct proxyOptions
{
    int drop_data_per;
    int drop_ack_per;
    char *sender_ip;
    char *receiver_ip;
    in_port_t port_in;
    in_port_t port_out;
};

static void parse_proxy_arguments(int argc, char *argv[], struct proxyOptions *opts);
static void options_init(struct proxyOptions *opts);

#define DEFAULT_PORT 5000

int main (int argc, char *argv[]) {
    struct proxyOptions opts;

    options_init(&opts);
    parse_proxy_arguments(argc, argv, &opts);


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
                opts->drop_data_per = optarg;
                break;
            }
            case 'a': //drop ack percentage
            {
                opts->drop_ack_per = optarg;
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
    opts->port  = DEFAULT_PORT;
}
