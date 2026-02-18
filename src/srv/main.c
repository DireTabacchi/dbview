#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "file.h"
#include "parse.h"
#include "srvpoll.h"

clientstate_t client_states[MAX_CLIENTS] = {0};

void print_usage(char* argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n  - create new database file\n");
    printf("\t -f  - (required) path to database file\n");
    printf("\t -p  - (required) port to listen to\n");
    return;
}

void poll_loop(unsigned short port, struct dbheader_t* dbhdr, struct employee_t* employees) {
    int listen_fd, conn_fd, free_slot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS+1];
    int nfds = 1;
    int opt = 1;

    // Initialize client states
    init_clients(client_states);

    // Create listening socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("listen socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {

    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("listen bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(listen_fd, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
        int ii = 1;
        // Add active connections to the read set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_states[i].fd != -1) {
                fds[ii].fd = client_states[i].fd; // Offset by 1 for listen_fd
                fds[ii].events = POLLIN;
                ii++;
            }
        }

        // Wait for an activity on one of the sockets
        int n_events = poll(fds, nfds, -1);
        if (n_events == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // Check for new connections
        if (fds[0].revents & POLLIN) {
            if ((conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len)) == -1) {
                perror("accept");
                continue;
            }

            printf("New connection from %s:%d\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // find a free slot for the new connection
            free_slot = find_free_slot(client_states);
            if (free_slot == -1) {
                printf("Server full: closing new connection\n");
                close(conn_fd);
            } else {
                client_states[free_slot].fd = conn_fd;
                client_states[free_slot].state = STATE_CONNECTED;
                nfds++;
                printf("Slot %d has fd %d\n", free_slot, client_states[free_slot].fd);
            }

            n_events--;
        }

        // Check each client for read/write activity
        for (int i = 1; i <= nfds && n_events > 0; i++) {
            if (fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot = find_slot_by_fd(client_states, fd);
                ssize_t bytes_read = read(client_states[slot].fd,
                    &client_states[slot].buffer, sizeof(client_states[slot].buffer));
                if (bytes_read <= 0) {
                    // Connection closed or error
                    close(fd);
                    if (slot == -1) {
                        printf("Tried to close fd that doesn't exist?\n");
                    } else {
                        client_states[slot].fd = -1;
                        client_states[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected or error\n");
                        nfds--;
                    }
                } else {
                    printf("Received data from client: %s\n", client_states[slot].buffer);
                }
            }
        }
    }

    //return 0;
}

int main(int argc, char* argv[]) {
    char* filepath = NULL;
    char* portstring = NULL;
    bool newfile = false;
    bool list = false;
    unsigned short port = 0;
    int c;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    while ((c = getopt(argc, argv, "nf:p:")) != -1) {
        switch (c) {
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'p':
                portstring = optarg;
                break;
            case '?':
                printf("Unknown option -%c\n", c);
                return -1;
            default:
                return -1;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is a required argument\n");
        print_usage(argv);
        return 0;
    }

    if (newfile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return -1;
        }

        if (create_db_header(&dbhdr) == STATUS_ERROR) {
            printf("Failed to create database header\n");
            return -1;
        }
    } else {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to open database file\n");
            return -1;
        }

        if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to validate database header\n");
            return -1;
        }
    }

    if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
        printf("Failed to read employees\n");
        return 0;
    }

    if (portstring) {
        port = strtoul(portstring, NULL, 10);
    }

    if (list) {
        if (list_employees(dbhdr, employees) != STATUS_SUCCESS) {
            printf("Unable to list employees\n");
            return 0;
        }
    }

    poll_loop(port, dbhdr, employees);

    output_file(dbfd, dbhdr, employees);

    return 0;
}
