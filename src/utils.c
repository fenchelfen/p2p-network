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
    char name[16];
    char ip[32];
    char port_s[5];
    memcpy((void *) name, (void *) peer->name, sizeof(name));
    memcpy((void *) ip, (void *) inet_ntoa(peer->meta.sin_addr), sizeof(ip));
    sprintf(port_s, "%d", ntohs(peer->meta.sin_port));
    char *string = malloc(sizeof(name) + sizeof(ip) + sizeof(port_s));

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
    memcpy((void *) &peers[peer_cnt++], (void *) &peer_struct, sizeof(peer_in));
}

void parse_incoming_peers(char *buf, int buf_size, int n)
{
}

void handle_1(int peer_sock_fd)
{
    char buf[128] = { 0 };
    int len = recv(peer_sock_fd, &buf, sizeof(buf), 0);
    
    printf("Received bytes\t:\t%d\n", len); // Alice keeps on sending 4 more byte than expected, weird

    join_peer(buf);
    printf("New acquaintance\t:\t%s:%u\n",
           inet_ntoa(peers[peer_cnt-1].meta.sin_addr), ntohs(peers[peer_cnt-1].meta.sin_port));

    int n = 0;
    len = recv(peer_sock_fd, &n, sizeof(n), 0); // Do I need any flags? Otherwise, change to read syscall
    printf("Number of incoming peers:\t%d\n", n);
    
    for (int i = 0; i < n; ++i) {
        memset(buf, '\0', sizeof(buf));
        recv(peer_sock_fd, buf, sizeof(buf), 0);
        puts(buf);
    }
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

