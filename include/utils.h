#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
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


FILE *out; // Where administrative information is printed
char *working_dir;

peer_in peers[8];
int peer_cnt;

int my_port;

char **files; // Store filenames here, dynamically
int file_cnt;

char personal_info[1024];

void *dispatcher(void *);
void *rqst_handler(void*);
void requester();
void init_peer(int *, char **);

void join_peer(char *);
void add_file(char *);
void handle_0(int);
void handle_1(int);

peer_in parse_data(char *);
char *get_string(peer_in *);
int get_file_size(FILE *);
char read_byte(FILE *, int);
int has_peer(peer_in *);
int has_file(char *);
void update_filenames(char *);

pthread_t threads[10];
int thread_cnt;

pthread_mutex_t mutex;
pthread_mutex_t mutex_files;

