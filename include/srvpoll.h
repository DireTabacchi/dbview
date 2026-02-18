#ifndef SRVPOLL_H
#define SRVPOLL_H

#include <poll.h>
#include "parse.h"

#define MAX_CLIENTS 256
#define PORT 8080
#define BUFF_SIZE 4096

typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_HELLO,
    STATE_MSG,
    STATE_GOODBYE
} state_e;

typedef struct {
    int fd;
    state_e state;
    char buffer[4096];
} clientstate_t;

void init_clients(clientstate_t* states);
int find_free_slot(clientstate_t* states);
int find_slot_by_fd(clientstate_t* states, int fd);
void handle_client_fsm(struct dbheader_t* dbhdr, struct employee_t* employees, clientstate_t* client);

#endif // SRVPOLL_H
