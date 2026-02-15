#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "common.h"
#include "parse.h"

int createDbHeader(int fd, struct dbheader_t **header_out) {
    struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("calloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *header_out = header;

    return STATUS_SUCCESS;
}

int validateDbHeader(int fd, struct dbheader_t **header_out) {
    if (fd < 0) {
        printf("Got a bad file descriptor\n");
        return STATUS_ERROR;
    }

    struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("calloc failed to create db header\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC) {
        printf("Improper header magic\n");
        free(header);
        return -1;
    }

    if (header->version != 1) {
        printf("Improper header version\n");
        free(header);
        return -1;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        printf("Corrupted database\n");
        free(header);
        return -1;
    }

    *header_out = header;

    return STATUS_SUCCESS;
}

void outputFile(int fd, struct dbheader_t* dbhdr) {
    if (fd < 0) {
        printf("Got a bad file descriptor\n");
        return;
        //return STATUS_ERROR;
    }

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(dbhdr->filesize);
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);

    lseek(fd, 0, SEEK_SET);

    write(fd, dbhdr, sizeof(struct dbheader_t));
}
