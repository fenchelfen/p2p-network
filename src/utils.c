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
    // addr_in.sin_addr.s_addr = INADDR_ANY; // zeros ip, y tho i dunno
    memcpy((void *) &peer.meta, (void *) &addr_in, sizeof(addr_in));
    memcpy((void *) peer.name, (void *) name, sizeof(peer.name));  
    return peer;
}

char *get_string(peer_in *peer)
{
    // char *string = malloc(sizeof(name) + sizeof(ip) + sizeof(port_s));
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

void join_peer(char *peer)
{
    peer_in peer_struct = parse_data(peer);
    if (is_present(&peer_struct)) {
        return;
    }
    printf("New acquaintance\t:\t%s %s:%u\n",
           peer_struct.name, inet_ntoa(peer_struct.meta.sin_addr), ntohs(peer_struct.meta.sin_port));
    memcpy((void *) &peers[peer_cnt++], (void *) &peer_struct, sizeof(peer_in));
}

void parse_incoming_peers(char *buf, int buf_size, int n)
{
}

/* Returns 1 if present */
int is_present(peer_in *peer)
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

void handle_0(int peer_sock_fd)
{
}

void handle_1(int peer_sock_fd)
{
    char buf[1024] = { 0 };
    int len = recv(peer_sock_fd, &buf, sizeof(buf), 0);
    
    printf("Received bytes\t\t:\t%d\n", len); // Alice keeps on sending 4 more byte than expected, weird

    int n = 0;
    len = recv(peer_sock_fd, &n, sizeof(n), 0); // Do I need any flags? Otherwise, change to read syscall
    printf("Number of incoming peers:\t%d\n", n);
    
    pthread_mutex_lock(&mutex); 
    join_peer(buf);
    for (int i = 0; i < n; ++i) {
        memset(buf, '\0', sizeof(buf));
        recv(peer_sock_fd, buf, sizeof(buf), 0);
        puts(buf);
        join_peer(buf);
    }
    puts("LIST PPEEER");
    for (int i = 0; i < peer_cnt; ++i) {
        printf("Peer\t:\t%s %s:%u\n",
               peers[i].name, inet_ntoa(peers[i].meta.sin_addr), ntohs(peers[i].meta.sin_port));

    }
    pthread_mutex_unlock(&mutex); 
}

char read_byte(char *filename, int n)
{
    char byte;
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Couldn't open %s", filename);
        perror("");
        exit(EXIT_FAILURE);
    }
    fseek(file, n, 0);
    fread(&byte, 1, 1, file);
    fclose(file);
    return byte;
}

