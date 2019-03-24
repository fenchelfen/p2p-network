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

typedef struct {
    char name[16];
    struct sockaddr_in meta; 
} peer_in;

peer_in peers[8];
int peer_cnt;

int my_port;

char files[8]; // Store filenames here, statically 
int file_cnt;

char *personal_info;

void *dispatcher(void *);
void *rqst_handler(void*);
void requester();
char read_byte(char *, int);
void init_peer(int *, char **);

void join_peer(char *);
void handle_1(int);

peer_in parse_data(char *);
char *get_string(peer_in *);

pthread_t threads[10];
int thread_cnt;

pthread_mutex_t mutex;

