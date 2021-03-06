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
    file_cnt = 0;
    my_port = strtol(argv[2], NULL, 10);
    int meta_port = strtol(argv[4], NULL, 10);

    working_dir = malloc(PATH_MAX*sizeof(char));
    getcwd(working_dir, PATH_MAX);

    files = malloc(20*sizeof(char *)); // todo: change to realloc
    update_filenames(working_dir);

    for(int k = 0; files[k] != NULL; ++k) {
        puts(files[k]);
    }

    cur_hosts = ht_create( 64 );
    blacklist = ht_create( 64 );

    memset(personal_info, '\0', strlen(personal_info));
    strcat(personal_info, "alice");
    strcat(personal_info, ":");
    strcat(personal_info, argv[1]);
    strcat(personal_info, ":");
    strcat(personal_info, argv[2]);
    strcat(personal_info, ":");
    strcat(personal_info, "\0");

    struct sockaddr_in meta;
    peer_in first_peer;
    memset(&meta, 0, sizeof(meta));
    memset(&first_peer, 0, sizeof(first_peer));
    strcpy(first_peer.name, "alice");
    meta.sin_family = AF_INET;
    meta.sin_port   = htons(meta_port);
    meta.sin_addr.s_addr  = inet_addr(argv[3]); // argv[3] is first peer's ip

    memcpy((void *) &first_peer.meta, (void *) &meta, sizeof(meta));
    memcpy((void *) &peers[peer_cnt++], (void *) &first_peer, sizeof(first_peer));

    struct sockaddr_in server_sock_in;
    memset(&server_sock_in, 0, sizeof(server_sock_in));

    server_sock_in.sin_family = AF_INET; // Socket internet, socket unix, socket link layer  
    server_sock_in.sin_port   = htons(my_port);
    server_sock_in.sin_addr.s_addr  = inet_addr(argv[1]); // argv[1] is server's ip

    fprintf(out, "Server's meta\t\t:\t%s:%u\n",
           inet_ntoa(server_sock_in.sin_addr), ntohs(server_sock_in.sin_port));
    fprintf(out, "First peer's meta\t:\t%s %s:%u\n\n",
           first_peer.name, inet_ntoa(meta.sin_addr), ntohs(meta.sin_port));

    if (setsockopt(*master_fd_ptr, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed"); 
    }
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
    struct sockaddr_in *client_sock_in = (struct sockaddr_in *) params[1];

    // LINUX : https://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval tv;
    tv.tv_sec = 4;
    tv.tv_usec = 0;
    setsockopt(peer_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv));

    fprintf(out, "Handler is dispatched\t:\t%ld\n", time(NULL));
    fprintf(out, "Thread's id          \t:\t%ld\n", syscall(__NR_gettid));
    fprintf(out, "Peer socket fild     \t:\t%d\n", peer_sock_fd);

    int opcode;
    for (;;) {
        int len = recv(peer_sock_fd, &opcode, sizeof(int), 0);
        if (len == -1) {
            fprintf(out, "Failed to recv OPСODE\t:\t%s\n"
                         "Close conn with\t\t:\t%s:%d\n", strerror(errno), inet_ntoa(client_sock_in->sin_addr), ntohs(client_sock_in->sin_port));
            dec_host_conn(inet_ntoa(client_sock_in->sin_addr));
            break;
        };
        if (len == 0) {
            fprintf(out, "Recved 0 bytes, it might be peer's orderly shutdown\n"
                         "Close conn with\t\t:\t%s:%d\n", inet_ntoa(client_sock_in->sin_addr), ntohs(client_sock_in->sin_port));
            dec_host_conn(inet_ntoa(client_sock_in->sin_addr));
            break;
        };
        
        fprintf(out, "OPCODE\t\t\t:\t%d\n", opcode);
        // if (opcode == -237) {
        //     fprintf(out, "Got a cycle,\n"
        //                  "Close conn with\t\t:\t%s:%d\n", inet_ntoa(client_sock_in->sin_addr), ntohs(client_sock_in->sin_port));
        //     dec_host_conn(inet_ntoa(client_sock_in->sin_addr));
        //     break;
        // }

        if (opcode == 237) {
            fprintf(out, "Conn closure rqst arrived\n"
                         "Release fild\t\t:\t%d\n", peer_sock_fd);
            dec_host_conn(inet_ntoa(client_sock_in->sin_addr));
            break;
        } else if (
            opcode == 0) {
            fprintf(out, "FGET from\t\t:\t%s:%d\n",
                    inet_ntoa(client_sock_in->sin_addr), ntohs(client_sock_in->sin_port));
            handle_0(peer_sock_fd);
        } else if (
            opcode == 1) {
            fprintf(out, "SYN from\t\t:\t%s:%d\n",
                    inet_ntoa(client_sock_in->sin_addr), ntohs(client_sock_in->sin_port));
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
    int addrlen = sizeof(client_sock_in);

    for (;;) {
        fd_set fd_collection;
        FD_ZERO(&fd_collection);
        FD_SET(master_fild, &fd_collection);
        if ((select(master_fild + 1, &fd_collection, NULL, NULL, NULL)) == -1) {
            perror("Failed to select");
        }
        
        if(FD_ISSET(master_fild, &fd_collection)) {

            memset(&client_sock_in, 0, addrlen);
            int peer_sock_fd = accept(master_fild, (struct sockaddr *) &client_sock_in, &addrlen);
            if (peer_sock_fd == -1) {
                perror("Failed to accept CIR");
                sleep(1);
                continue;
            }
            char *key = inet_ntoa(client_sock_in.sin_addr);

            if (is_blacklisted(key)) {
                fprintf(out, "Blacklisted, close conn\t:\t%s:%u\n",
                             inet_ntoa(client_sock_in.sin_addr), ntohs(client_sock_in.sin_port));
                close(peer_sock_fd); 
                continue;
            }

            puts("Flag B");

            inc_host_conn(key);
            puts("Flag C");
            if (ht_get( cur_hosts, key ) >= 3) {
                add_to_blacklist(key);
            }

            fprintf(out, "Established new conn\t:\t%s:%u\t:%d\n",
                         inet_ntoa(client_sock_in.sin_addr), ntohs(client_sock_in.sin_port), ht_get( cur_hosts, key ));

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
    for (;;) {
        char filename[1024];
        puts("Which file do you want?\t:\t");
        scanf("%s", filename);

        if (strcmp(filename, "-") == 0) {
            puts("Exit RQSTer thread\n"
                 "Press Ctrl-C to slay dispatcher\n");
            break;
        }

        for(int j = 0; j < peer_cnt; ++j) {

            int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            struct timeval tv;
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv));

            struct sockaddr_in dest;
            memcpy((void *) &dest, (void *) &peers[j].meta, sizeof(peers[j].meta));
            int addr_len = sizeof(dest);

            if (connect(socket_fd, (struct sockaddr *) &dest, addr_len) == - 1) {
                fprintf(out, "Failed to TCP peer\t:\t%s:%u\n"
                             "%s\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), strerror(errno));
                continue;
            };
            
            if (strcmp(filename, "_sync") == 0) {
                fprintf(out, "SYN with\t\t\t%s:%u\n",
                             inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
                _sync(socket_fd);
            } else {
                fprintf(out, "FGET file %s from\t:\t%s:%u\n", filename,
                             inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
                _fget(socket_fd, filename);
            }

            send(socket_fd, &(int){ 237 }, sizeof(int), 0);
            break;
        }
    }
    
    fprintf(out, "Thread cnt when\n"
                 "join is reached\t\t:\t%d\n", thread_cnt);
    for (int i = 0; i < thread_cnt; ++i) {
        pthread_join(threads[i], NULL);
    }
    free(files);
    free(working_dir);
}
    
