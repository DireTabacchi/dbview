#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "common.h"
#include "srvpoll.h"

void init_clients(clientstate_t* states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        states[i].fd = -1;
        states[i].state = STATE_NEW;
        memset(&states[i].buffer, '\0', BUFF_SIZE);
    }
}

// Find a free slot in the states array.
// Return the index into the states array if a free slot is found,
// otherwise return -1.
int find_free_slot(clientstate_t* states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

// Find the slot that fd occupies.
// Return the index into the states array if fd is found,
// otherwise return -1;
int find_slot_by_fd(clientstate_t* states, int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == fd) {
            return i;
        }
    }
    return -1;
}

void fsm_reply_hello(clientstate_t* client, dbproto_hdr_t* hdr) {
    hdr->type = htonl(MSG_HELLO_RESP);
    hdr->len = htons(1);
    dbproto_hello_resp* hello = (dbproto_hello_resp*)&hdr[1];
    hello->proto = htons(PROTO_VER);

    write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_resp));
}

void fsm_reply_hello_err(clientstate_t* client, dbproto_hdr_t* hdr) {
    hdr->type = htonl(MSG_ERROR);
    hdr->len = htons(0);

    write(client->fd, hdr, sizeof(dbproto_hdr_t));
}

void handle_client_fsm(
    struct dbheader_t* dbhdr,
    struct employee_t* employees,
    clientstate_t* client
) {
    dbproto_hdr_t* hdr = (dbproto_hdr_t*)client->buffer;

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (client->state == STATE_HELLO) {
        if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
            printf("Didn't get MSG_HELLO_REQ in STATE_HELLO...\n");
            // TODO: send error message
        }

        dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
        hello->proto = ntohs(hello->proto);
        if (hello->proto != PROTO_VER) {
            printf("Protocol mismatch...\n");
            fsm_reply_hello_err(client, hdr);
            return;
        }

        fsm_reply_hello(client, hdr);
        client->state = STATE_MSG;
        printf("Client transitioned to STATE_MSG\n");
        return;
    }

    if (client->state == STATE_MSG) {
    }
}
