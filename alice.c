#include "utils.h"


int main(int argc, char **argv)
{
   if (argc < 5) {
        puts("Your ip,\n"
             "Your port,\n"
             "First peer ip,\n"
             "First peer port,\n"
             "Initial file\n");
        exit(EXIT_FAILURE);
    }
    int master_fild = 0;
    init_peer(&master_fild, argv);
    pthread_t dispatcher_t;
    pthread_create(&threads[thread_cnt++], NULL, &dispatcher, &master_fild);
    requester();
    return 0;
}

void init_peer(int *master_fd_ptr, char **argv)
{
    if ((*master_fd_ptr = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("Socket creation failed");
        return;
    }
    thread_cnt = 0;
    peer_cnt = 0;
    file_cnt = 0;
    my_port = strtol(argv[2], NULL, 10);
    int first_peer_port = strtol(argv[4], NULL, 10);

    personal_info = malloc(sizeof("alice") + sizeof(argv[1]) + sizeof(argv[2]) + 3); // 3 is for colons
    memset(personal_info, '\0', sizeof(personal_info));
    strcat(personal_info, "alice");
    strcat(personal_info, ":");
    strcat(personal_info, argv[1]);
    strcat(personal_info, ":");
    strcat(personal_info, argv[2]);
    strcat(personal_info, ":");

    puts(personal_info);

    // Add init file to the list, argv[5] is the init file's name
    memcpy((void *) &files[file_cnt++], argv[5], sizeof(argv[4]));

    // Add first peer to the list
    struct sockaddr_in first_peer;
    memset(&first_peer, 0, sizeof(first_peer));
    first_peer.sin_family = AF_INET;
    first_peer.sin_addr.s_addr  = inet_addr(argv[3]); // argv[3] is first peer's ip
    first_peer.sin_port   = htons(first_peer_port);
    // first_peer.sin_addr.s_addr = INADDR_ANY; // zeros ip; i dunno y tho
    printf("First peer's everything\t:\t%s:%u\n",
           inet_ntoa(first_peer.sin_addr), ntohs(first_peer.sin_port));

    memcpy((void *) &peers[peer_cnt++], (void *) &first_peer, sizeof(struct sockaddr_in));

    struct sockaddr_in server_sock_in;
    memset(&server_sock_in, 0, sizeof(server_sock_in));

    server_sock_in.sin_family = AF_INET; // Socket internet, socket unix, socket link layer  
    server_sock_in.sin_port   = htons(my_port);
    server_sock_in.sin_addr.s_addr  = inet_addr(argv[1]); // argv[1] is server's ip
    // server_sock_in.sin_addr.s_addr = INADDR_ANY; // Means, socket is bound to all local interfaces
    printf("Server's everything\t:\t%s:%u\n",
           inet_ntoa(server_sock_in.sin_addr), ntohs(server_sock_in.sin_port));
    puts("");        

    if (setsockopt(*master_fd_ptr, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if (bind(*master_fd_ptr, (struct sockaddr *) &server_sock_in, sizeof(struct sockaddr)) == -1) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(*master_fd_ptr, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
}


void *rqst_handler(void *args)
{
    void **params = (void **) args;
    int peer_sock_fd = *((int *) params[0]);

    printf("Handler is dispatched\t:\t%ld\n", time(NULL));
    printf("Thread's id          \t:\t%ld\n", syscall(__NR_gettid));
    printf("Peer socket fild     \t:\t%d\n", peer_sock_fd);

    struct sockaddr_in *client_sock_in = (struct sockaddr_in *) params[1];
    // packet message; 
    int opcode;

    for (;;) {
        int len = recv(peer_sock_fd, &opcode, sizeof(int), 0); // Do I need any flags? Otherwise, change to read syscall

        printf("OPCODE\t:\t%d\n", opcode);

        if (opcode == 237) {
            printf("Conn closure rqst arrived;\n"
                   "Release fild\t:\t%d\n", peer_sock_fd);
            break;
        } else if (
            opcode == 1) {
            handle_1(peer_sock_fd);
        }
        // else if (
        //     strcmp(message.cmd, "JOIN") == 0) {
        //     printf("JOIN rqst from\t%s:%u\n",
        //            inet_ntoa(message.meta.sin_addr), ntohs(message.meta.sin_port));

        //     memcpy((void *) &peers[peer_cnt++], (void *) &message.meta, sizeof(message.meta));
        // } else if (
        //     strcmp(message.cmd, "QUER") == 0) {
        //     // Compare against your list of files
        // } else if (
        //     strcmp(message.cmd, "LIST") == 0) {
        //     send(peer_sock_fd, &peers, sizeof(peers), 0);
        // } else if (
        //     strcmp(message.cmd, "FGET") == 0) {
        //     char byte = read_byte(message.filename, message.byte_n);
        //     send(peer_sock_fd, &byte, 1, 0);
        // }
    }
    free(client_sock_in);
    close(peer_sock_fd);
    --thread_cnt;
    puts("Bye");
    pthread_exit(NULL);
}

void *dispatcher(void *master_fild_ptr)
{
    int master_fild = *((int *) master_fild_ptr);
    struct sockaddr_in client_sock_in;
    memset(&client_sock_in, 0, sizeof(client_sock_in));
    int addrlen = sizeof(client_sock_in); // Define up front for later use in accept

    for (;;) {
        fd_set fd_collection;
        FD_ZERO(&fd_collection); // Clear the set
        FD_SET(master_fild, &fd_collection); // Add a new fd to the set

        select(master_fild + 1, &fd_collection, NULL, NULL, NULL); // nfds is one more than the max value in fd_set; thus, we increment
        
        if(FD_ISSET(master_fild, &fd_collection)) {
            puts("CIR arrived, try to accept...");

            memset(&client_sock_in, 0, addrlen);
            int peer_sock_fd = accept(master_fild, (struct sockaddr *) &client_sock_in, &addrlen);

            if (peer_sock_fd == -1) {
                perror("Failed to accept CIR");
                sleep(1);
                continue;
            }

            printf("Established new conn\t:\t%s:%u\n",
                   inet_ntoa(client_sock_in.sin_addr), ntohs(client_sock_in.sin_port));
            
            void **params[2];
            params[0] = (void *) &peer_sock_fd;
            params[1] = malloc(sizeof(client_sock_in));
            memcpy(params[1], (void *) &client_sock_in, sizeof(client_sock_in));

            pthread_create(&threads[thread_cnt++], NULL, &rqst_handler, &params);
            // !!! Handle if breaks
        }
    }
    close(master_fild); // Is that eventually closed?
    pthread_exit(NULL);
}

void *retr_byte_handler(void *args)
{
}

void requester()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in dest;
    memcpy((void *) &dest, (void *) &peers[0], sizeof(peers[0]));
    int addr_len = sizeof(dest);

    sleep(2); // Give the dispatcher time to start. Premature termination avoidance 
    
    // TCP the first peer
    if (connect(socket_fd, (struct sockaddr *) &dest, addr_len) == - 1) {
        perror("Failed to TCP initial peer");
        exit(EXIT_FAILURE);
    };

    send(socket_fd, &(int){ 1 }, sizeof(int), 0);

    puts("");
    send(socket_fd, personal_info, strlen(personal_info), 0);
    send(socket_fd, &(int){ 237 }, sizeof(int), 0);

    /* Issue: if the server thread is not yet dispatched, premature join may occure sry ... */
    
    printf("Thread cnt when join is reached\t:\t%d\n", thread_cnt);
    for (int i = 0; i < thread_cnt; ++i) {
        pthread_join(threads[i], NULL);
    }
    // Init a separate socket
    // LIST
    // JOIN
    // for each peer:
    // QUER if the file is present
    // true -> FGET i-th byte, 
}
