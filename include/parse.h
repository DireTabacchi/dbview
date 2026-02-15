#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC 0x4c4c4144

struct dbheader_t {
    unsigned int magic;
    unsigned short version;
    unsigned short count;
    unsigned int filesize;
};

struct employee_t {
    char name[256];
    char address[256];
    unsigned int hours;
};

int createDbHeader(int fd, struct dbheader_t **header_out);
int validateDbHeader(int fd, struct dbheader_t **header_out);
int readEmployees(int fd, struct dbheader_t*, struct employee_t **employees_out);
void outputFile(int fd, struct dbheader_t*);

#endif // PARSE_H
