#include "../include/utils.h"


peer_in parse_data(char *string)
{
    peer_in peer;
    struct sockaddr_in addr_in;
    char name[16] = { 0 };
    char ip[32] = { 0 };
    char port_s[5] = { 0 };
    char filenames[64] = { 0 };
    
    sscanf(string, "%[^:]:%[^:]:%[^:]:%s", name, ip, port_s, filenames);
    int port = strtol(port_s, NULL, 10);
    memset(&addr_in, 0, sizeof(addr_in));
    memset(&peer, 0, sizeof(peer));
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = inet_addr(ip);
    addr_in.sin_port   = htons(port);
    memcpy((void *) &peer.meta, (void *) &addr_in, sizeof(addr_in));
    memcpy((void *) peer.name, (void *) name, sizeof(peer.name));  
    return peer;
}

char *get_string(peer_in *peer)
{
    char *string = malloc(sizeof(char)*1024);
    memset(string, '\0', 1024);
    char name[16];
    char ip[32];
    char port_s[5];
    memcpy((void *) name, (void *) peer->name, sizeof(name));
    memcpy((void *) ip, (void *) inet_ntoa(peer->meta.sin_addr), sizeof(ip));
    sprintf(port_s, "%d", ntohs(peer->meta.sin_port));

    strcat(string, name);
    strcat(string, ":");
    strcat(string, ip);
    strcat(string, ":");
    strcat(string, port_s);
    strcat(string, ":");
    strcat(string, "\0");
    return string;
}

void add_file(char *filename)
{
    if (has_file(filename)) {
        pthread_mutex_unlock(&mutex_files);
        return;
    }
    pthread_mutex_lock(&mutex_files);
    files[file_cnt] = malloc(sizeof(char)*strlen(filename));
    strcpy(files[file_cnt], filename);
    files[++file_cnt] = NULL;
    pthread_mutex_unlock(&mutex_files);
}

void join_peer(char *peer)
{
    pthread_mutex_lock(&mutex); 
    peer_in peer_struct = parse_data(peer);
    if (has_peer(&peer_struct) || strcmp(peer, personal_info) == 0) {
        pthread_mutex_unlock(&mutex); 
        return;
    }
    fprintf(out, "New acquaintance\t:\t%s %s:%u\n",
           peer_struct.name, inet_ntoa(peer_struct.meta.sin_addr), ntohs(peer_struct.meta.sin_port));
    memcpy((void *) &peers[peer_cnt++], (void *) &peer_struct, sizeof(peer_in));
    pthread_mutex_unlock(&mutex); 
}

int has_peer(peer_in *peer)
{
    for (int i = 0; i < peer_cnt; ++i) {
        if (strcmp(inet_ntoa(peer->meta.sin_addr), inet_ntoa(peers[i].meta.sin_addr)) == 0 &&
            ntohs(peer->meta.sin_port) == ntohs(peers[i].meta.sin_port)) {
            memset(peers[i].name, '\0', sizeof(peers[i].name));
            strcpy(peers[i].name, peer->name);
            return 1;
        }
    }
    return 0;
}

int has_file(char *string) {
    for (int i = 0; i < file_cnt; ++i) {
        if (strcmp(string, files[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void handle_0(int peer_sock_fd)
{
    char buf[1024] = { 0 };
    int len = recv(peer_sock_fd, &buf, sizeof(buf), 0);

    if (!has_file(buf)) {
        send(peer_sock_fd, &(int){ -1 }, sizeof(int), 0);
        return;
    }
    fprintf(out, "RQSTed file %s\n", buf);

    FILE *file = fopen(buf, "rb");
    if (file == NULL) {
        fprintf(out, "Filed to open %s\n", buf);
        fprintf(out, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fprintf(out, "Start sending %s\n", buf);
    int f_size = get_file_size_in_words(file);
    fprintf(out, "File size is %d\n", f_size);
    send(peer_sock_fd, &f_size, sizeof(f_size), 0);

    for (int i = 0; i < f_size; ++i) {
         char *word = read_word(file, i);
         puts(word);
         send(peer_sock_fd, word, strlen(word)+1, 0);
         free(word);
    }
}

void handle_1(int peer_sock_fd)
{
    char buf[1024] = { 0 };
    if (recv(peer_sock_fd, &buf, sizeof(buf), 0) == -1) {
        return;
    }
    int n = 0;
    if (recv(peer_sock_fd, &n, sizeof(n), 0) == -1) {
        return;
    }

    fprintf(out, "Number of incoming peers:\t%d\n", n);
    
    join_peer(buf);
    for (int i = 0; i < n; ++i) {
        memset(buf, '\0', sizeof(buf));
        if (recv(peer_sock_fd, buf, sizeof(buf), 0) == -1) {
            return;
        }
        join_peer(buf);
    }
    fprintf(out, "List of my peers\n");
    for (int i = 0; i < peer_cnt; ++i) {
        fprintf(out, "Peer\t\t\t:\t%s %s:%u\n",
                peers[i].name, inet_ntoa(peers[i].meta.sin_addr), ntohs(peers[i].meta.sin_port));
    }
}

void _sync(int socket_fd)
{
    send(socket_fd, &(int){ 1 }, sizeof(int), 0);
    sleep(1);
    send(socket_fd, personal_info, strlen(personal_info), 0);
    sleep(1);

    send(socket_fd, &peer_cnt, sizeof(peer_cnt), 0);
    sleep(1);
    for (int i = 0; i < peer_cnt; ++i) {
        char *peer_str = get_string(&peers[i]);
        fprintf(out, "Send peer\t\t:\t%s\n", peer_str);
        send(socket_fd, peer_str, strlen(peer_str), 0); 
        free(peer_str);
        sleep(1);
    }
}

int _fget(int socket_fd, char *filename)
{
     send(socket_fd, &(int){ 0 }, sizeof(int), 0);
     sleep(1);
     send(socket_fd, filename, strlen(filename), 0);
     sleep(1);
 
     int file_len = 0;
     if (recv(socket_fd, &file_len, sizeof(int), 0) == -1) {
         fprintf(out, "Failed to recv file length,\n"
                      "%s\n", strerror(errno));
         return -1;
     }

     fprintf(out, "Incoming file length\t:\t%d\n", file_len);
     
     if (file_len == -1) {
         send(socket_fd, &(int){ 237 }, sizeof(int), 0);
         return -1;
     }
     
     umask(0);
     // O_APPEND | 
     int file = open(filename, O_CREAT | O_WRONLY, 0666);
     int k = 0;
     for (; k < file_len;) {
         char byte = 0;
         recv(socket_fd, &byte, sizeof(byte), 0);
         putchar(byte);
         putchar('\n');
         printf("Cnt\t\t\t:\t%d\n", k);
         if (byte == '\0') {
             ++k;
             write(file, &(int){ ' ' }, sizeof(char));
             continue;
         }
         write(file, &byte, sizeof(byte));
     }
     add_file(filename);
     close(file);
}


void add_to_blacklist(char *key)
{
    pthread_mutex_lock(&mutex_blacklist); 
    ht_set( blacklist, key, 1); 
    pthread_mutex_unlock(&mutex_blacklist); 
}

int is_blacklisted(char *key)
{
    if (ht_get( blacklist, key) == -237) {
        return 0;
    }
    return 1;
}

void inc_host_conn(char *key)
{
    pthread_mutex_lock(&mutex_cur_hosts); 
    if (ht_get( cur_hosts, key) == -237) {
        ht_set( cur_hosts, key, 0); 
    } else {
        ht_set( cur_hosts, key, ht_get( cur_hosts, key ) + 1); 
    }
    pthread_mutex_unlock(&mutex_cur_hosts); 
}

void dec_host_conn(char *key)
{
    pthread_mutex_lock(&mutex_cur_hosts); 
    if (ht_get( cur_hosts, key) == -237) {
        ht_set( cur_hosts, key, 0); 
    } else {
        if (ht_get( cur_hosts, key) != 0) { 
            ht_set( cur_hosts, key, ht_get( cur_hosts, key ) - 1); 
        }
    }
    pthread_mutex_unlock(&mutex_cur_hosts); 
}


int get_file_size(FILE *file)
{
    fseek(file, 0L, SEEK_END);
    int n = ftell(file);
    rewind(file);
    return n;
}

int get_file_size_in_words(FILE *file)
{
    char buf[512];
    int k = 0;
    while ((fscanf(file, "%s", buf)) != EOF) {
        ++k;
    }
    rewind(file);
    return k;
}

char read_byte(FILE *file, int n)
{
    char byte;
    fseek(file, n, 0);
    fread(&byte, 1, 1, file);
    return byte;
}

char *read_word(FILE *file, int n)
{
    char *word;
    char buf[512] = { 0 };
    int k = 0;
    while ((fscanf(file, "%s", buf)) != EOF && k < n) {
        ++k;
    }
    word = malloc(sizeof(char)*(strlen(buf)+1));
    strcpy(word, buf);
    rewind(file);
    return word;
}

void update_filenames(char *source)
{
    DIR *d = opendir(source);;
    struct dirent *dir;
    int k = 0;
    if (d) {
        while((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 ||
                strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            add_file(dir->d_name);
        }
    }
    closedir(d);
}

