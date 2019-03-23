#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>
#include <memory.h>
#include <err.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <syscall.h>
#include <time.h>

struct sockaddr_in peers[8];
int peer_cnt;

int my_port;

char files[8]; // Store filenames here, statically 
int file_cnt;

char *personal_info;

typedef struct {
    char cmd[5];
    char filename[10];
    int byte_n;
    struct sockaddr_in meta;
} packet;

typedef struct {
    char name[16];
    struct sockaddr_in meta; 
} peer_in;

void *dispatcher(void *);
void *rqst_handler(void*);
void requester();
char read_byte(char *, int);
void init_peer(int *, char **);

void handle_1(int);

peer_in parse_data(char *);

pthread_t threads[10];
int thread_cnt;

pthread_mutex_t mutex;

