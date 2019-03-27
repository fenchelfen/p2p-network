#include "../include/utils.h"


int main(int argc, char **argv)
{
    if (argc < 6) {
         puts("Your ip,\n"
              "Your port,\n"
              "First peer ip,\n"
              "First peer port,\n"
              "Log output\n");
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
    if (strcmp(argv[5], "stdout") == 0) {
        out = stdout;
    } else {
        out = fopen(argv[5], "w");
    }
    setbuf(out, NULL);
    thread_cnt = 0;
    peer_cnt = 0;
    my_port = strtol(argv[2], NULL, 10);
    int meta_port = strtol(argv[4], NULL, 10);

    working_dir = malloc(PATH_MAX*sizeof(char));
    getcwd(working_dir, PATH_MAX);

    file_cnt = 0;
    files = malloc(20*sizeof(char *));
    update_filenames(working_dir);
    int k = 0;
    while(files[k] != NULL) {
        puts(files[k++]);
    }

    // personal_info = malloc(sizeof("alice") + sizeof(argv[1]) + sizeof(argv[2]) + 3); // 3 is for colons
    memset(personal_info, '\0', strlen(personal_info));
    strcat(personal_info, "alice");
    strcat(personal_info, ":");
    strcat(personal_info, argv[1]);
    strcat(personal_info, ":");
    strcat(personal_info, argv[2]);
    strcat(personal_info, ":");
    strcat(personal_info, "\0");


    // // Add init file to the list, argv[6] is the init file's name
    // memcpy((void *) &files[file_cnt++], argv[6], sizeof(argv[6]));

    // Add first peer to the list
    struct sockaddr_in meta;
    peer_in first_peer;
    memset(&meta, 0, sizeof(meta));
    memset(&first_peer, 0, sizeof(first_peer));
    strcpy(first_peer.name, "alice");
    meta.sin_family = AF_INET;
    meta.sin_addr.s_addr  = inet_addr(argv[3]); // argv[3] is first peer's ip
    meta.sin_port   = htons(meta_port);
    // meta.sin_addr.s_addr = INADDR_ANY; // zeros ip; i dunno y tho
    fprintf(out, "First peer's meta\t:\t%s %s:%u\n",
           first_peer.name, inet_ntoa(meta.sin_addr), ntohs(meta.sin_port));

    memcpy((void *) &first_peer.meta, (void *) &meta, sizeof(meta));
    memcpy((void *) &peers[peer_cnt++], (void *) &first_peer, sizeof(first_peer));

    struct sockaddr_in server_sock_in;
    memset(&server_sock_in, 0, sizeof(server_sock_in));

    server_sock_in.sin_family = AF_INET; // Socket internet, socket unix, socket link layer  
    server_sock_in.sin_port   = htons(my_port);
    server_sock_in.sin_addr.s_addr  = inet_addr(argv[1]); // argv[1] is server's ip
    // server_sock_in.sin_addr.s_addr = INADDR_ANY; // Means, socket is bound to all local interfaces
    fprintf(out, "Server's meta\t\t:\t%s:%u\n\n",
           inet_ntoa(server_sock_in.sin_addr), ntohs(server_sock_in.sin_port));

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

    fprintf(out, "Handler is dispatched\t:\t%ld\n", time(NULL));
    fprintf(out, "Thread's id          \t:\t%ld\n", syscall(__NR_gettid));
    fprintf(out, "Peer socket fild     \t:\t%d\n", peer_sock_fd);

    struct sockaddr_in *client_sock_in = (struct sockaddr_in *) params[1];
    int opcode;

    for (;;) {
        int len = recv(peer_sock_fd, &opcode, sizeof(int), 0); // Do I need any flags? Otherwise, change to read syscall
        fprintf(out, "OPCODE\t\t\t:\t%d\n", opcode);
        if (opcode == 237) {
            fprintf(out, "Conn closure rqst arrived;\n"
                   "Release fild\t\t:\t%d\n", peer_sock_fd);
            break;
        } else if (
            opcode == 0) {
            handle_0(peer_sock_fd);
        } else if (
            opcode == 1) {
            handle_1(peer_sock_fd);
        }
    }
    free(client_sock_in);
    close(peer_sock_fd);
    --thread_cnt;
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

        int e = select(master_fild + 1, &fd_collection, NULL, NULL, NULL); // nfds is one more than the max value in fd_set; thus, we increment
        if (e == -1) { perror("Failed to select"); }
        
        if(FD_ISSET(master_fild, &fd_collection)) {
            fprintf(out, "CIR arrived, try to accept...\n");

            memset(&client_sock_in, 0, addrlen);
            int peer_sock_fd = accept(master_fild, (struct sockaddr *) &client_sock_in, &addrlen);

            if (peer_sock_fd == -1) {
                perror("Failed to accept CIR");
                sleep(1);
                continue;
            }

            fprintf(out, "Established new conn\t:\t%s:%u\n",
                   inet_ntoa(client_sock_in.sin_addr), ntohs(client_sock_in.sin_port));
            
            void **params[2];
            params[0] = (void *) &peer_sock_fd;
            params[1] = malloc(sizeof(client_sock_in));
            memcpy(params[1], (void *) &client_sock_in, sizeof(client_sock_in));

            pthread_create(&threads[thread_cnt++], NULL, &rqst_handler, &params);
        }
    }
    close(master_fild); // Is that eventually closed? No, it's not
    pthread_exit(NULL);
}

void requester()
{
    sleep(2); // Give the dispatcher time to start. Premature termination avoidance 
    for (;;) {
        char filename[1024];
        puts("Which file do you want?\t:\t");
        scanf("%s", filename);

        for(int j = 0; j < peer_cnt; ++j) {
            int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            struct sockaddr_in dest;
    
            memcpy((void *) &dest, (void *) &peers[j].meta, sizeof(peers[j].meta));
            int addr_len = sizeof(dest);

            fprintf(out, "RQST file %s\t:\t%s:%u\n", filename,
                   inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
 

            if (connect(socket_fd, (struct sockaddr *) &dest, addr_len) == - 1) {
                perror("Failed to TCP initial peer");
                exit(EXIT_FAILURE);
            };
            
            if ((send(socket_fd, &(int){ 1 }, sizeof(int), 0)) == -1)             { perror("Failed to send"); }
            usleep(100000); // Give him time to process the rqst
            if ((send(socket_fd, personal_info, strlen(personal_info), 0)) == -1) { perror("Failed to send"); }
            usleep(100000); // Give him time to process the rqst
            
            send(socket_fd, &peer_cnt, sizeof(peer_cnt), 0);
            pthread_mutex_lock(&mutex); 
            for (int i = 0; i < peer_cnt; ++i) {
                char *peer_str = get_string(&peers[i]);
                fprintf(out, "Send peer\t\t:\t%s\n", peer_str);
                send(socket_fd, peer_str, strlen(peer_str), 0); 
                free(peer_str);
                usleep(100000);
            }
            pthread_mutex_unlock(&mutex); 
        
            if (strcmp(filename, "-") == 0) {
                break;
            }
            send(socket_fd, &(int){ 0 }, sizeof(int), 0);
            usleep(100000);
            send(socket_fd, filename, sizeof(filename), 0);
        
            int file_len = 0;
            recv(socket_fd, &file_len, sizeof(int), 0);
            
            if (file_len == -1) {
                send(socket_fd, &(int){ 237 }, sizeof(int), 0);
                continue;
            }
            // fprintf(out, "File len is\t\t\t:\t%d\n", file_len);
            
            int file = open(filename, O_APPEND | O_CREAT | O_WRONLY, 0666);
            if (file_len != -1) {
                for (int i = 0; i < file_len; ++i) {
                    char byte = 0;
                    recv(socket_fd, &byte, sizeof(char), 0);
                    putchar(byte);
                    write(file, &byte, sizeof(byte));
                    fflush(stdout);
                }
            }
            add_file(filename);
            puts("");
            close(socket_fd);
        }
    }
    // sleep(3); // Otherwise, 237 may be treated as a part of some other peer
    /* Issue: if the server thread is not yet dispatched, premature join may occure sry */
    
    fprintf(out, "Thread cnt when join is reached\t:\t%d\n", thread_cnt);
    for (int i = 0; i < thread_cnt; ++i) {
        pthread_join(threads[i], NULL);
    }
    free(files);
    free(working_dir);
}
    
