#include "utils.h"


struct sockaddr_in parse_data(char *string)
{
    struct sockaddr_in peer_in;
    char name[16] = { 0 };
    char ip[32] = { 0 };
    char port_s[5] = { 0 };
    char filenames[64] = { 0 };
    
    sscanf(string, "%[^:]:%[^:]:%[^:]:%s", name, ip, port_s, filenames);
    int port = strtol(port_s, NULL, 10);
    memset(&peer_in, 0, sizeof(peer_in));
    peer_in.sin_family = AF_INET;
    peer_in.sin_addr.s_addr = inet_addr(ip);
    peer_in.sin_port   = htons(port);
    // peer_in.sin_addr.s_addr = INADDR_ANY; // zeros ip, y tho i dunno
    return peer_in;
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

