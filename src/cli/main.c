#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"

int send_hello(int fd) {
    char buf[4096] = {0};

    dbproto_hdr_t* hdr = (dbproto_hdr_t*)buf;
    hdr->type = MSG_HELLO_REQ;
    hdr->len = 1;

    dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
    hello->proto = PROTO_VER;

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);
    hello->proto = htons(hello->proto);

    write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));

    read(fd, buf, sizeof(buf));

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    // handle error response
    if (hdr->type == MSG_ERROR) {
        printf("Protocol mismatch. Aborting.\n");
        close(fd);
        return STATUS_ERROR;
    }

    printf("Server connected, protocol v1.\n");
    return STATUS_SUCCESS;
}

void print_usage(char* argv) {
    printf("Usage: %s -h <host ip> -p <port number> [-a <employee_string>]\n", argv);
}

int main(int argc, char* argv[]) {
    char* addarg = NULL;
    char* portarg = NULL, * hostarg = NULL;
    unsigned short port = 0;

    int c;
    while((c = getopt(argc, argv, "p:h:a:")) != -1) {
        switch (c) {
        case 'p':
            portarg = optarg;
            port = atoi(portarg);
            break;
        case 'h':
            hostarg = optarg;
            break;
        case 'a':
            addarg = optarg;
            break;
        case '?':
            printf("Unknown option -%c\n", c);
        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    if (port == 0) {
        printf("Bad port: %s\n", portarg);
        print_usage(argv[0]);
        return -1;
    }

    if (hostarg == NULL) {
        printf("Must specify host ip\n");
        print_usage(argv[0]);
        return -1;
    }

    struct sockaddr_in server_info = {0};
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(hostarg);
    server_info.sin_port = htons(port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    if (connect(fd, (struct sockaddr*)&server_info, sizeof(server_info)) == -1) {
        perror("connect");
        return 0;
    }

    if (send_hello(fd) != STATUS_SUCCESS) {
        close(fd);
        return -1;
    }

    close(fd);
}
