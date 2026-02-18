#include <string.h>
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
