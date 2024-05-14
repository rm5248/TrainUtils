/* SPDX-License-Identifier: GPL-2.0 */

// Test the implementation by creating a simple TCP server and having the olcbchecker connect to it.
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include "lcc.h"
#include "lcc-gridconnect.h"

static void frame_parsed(struct lcc_gridconnect* gc, struct lcc_can_frame* frame){
    struct lcc_context* ctx = lcc_gridconnect_user_data(gc);

    lcc_context_incoming_frame(ctx, frame);
}

static int outgoing_frame(struct lcc_context* ctx, struct lcc_can_frame* frame){
    int* socket = lcc_context_user_data(ctx);
    char out_buffer[128];

    lcc_canframe_to_gridconnect(frame, out_buffer, sizeof(out_buffer));

    printf("Write %d bytes: %s\n", strlen(out_buffer), out_buffer);
    if(write(*socket, out_buffer, strlen(out_buffer)) < 0){
        perror("write");
    }

    return LCC_OK;
}

static int outgoing_frame_discard(struct lcc_context* ctx, struct lcc_can_frame* frame){
    return LCC_OK;
}

int main(int argc, char** argv){
    int socket_fd = -1;
    int client_fd = -1;
    struct lcc_context* ctx = NULL;
    struct lcc_gridconnect* gridconnect = NULL;

    ctx = lcc_context_new();
    gridconnect = lcc_gridconnect_new();

    // Before we do anything, get into a 'good' state.
    // We pretend that we have gotten a good alias by generating an alias
    lcc_context_set_write_function(ctx, outgoing_frame_discard, NULL);
    lcc_context_set_unique_identifer(ctx, 0x00000011);
    lcc_context_generate_alias(ctx);
    lcc_context_claim_alias(ctx);

    lcc_gridconnect_set_userdata(gridconnect, ctx);
    lcc_gridconnect_set_frame_parsed(gridconnect, frame_parsed);

    lcc_context_set_write_function(ctx, outgoing_frame, NULL);
    lcc_context_set_userdata(ctx, &client_fd);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_fd < 0){
        perror("socket");
        return 1;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    getaddrinfo("localhost", "12021", &hints, &res);

    for( struct addrinfo* current = res; current != NULL; current = current->ai_next ){
        if(bind(socket_fd, current->ai_addr, current->ai_addrlen) < 0){
            perror("bind");
        }else{
            break;
        }
    }

    if(listen(socket_fd, 1) < 0){
        perror("listen");
        return 1;
    }

    // Okay, now we listen for a single connection and run any appropriate tests
    client_fd = accept(socket_fd, NULL, NULL);
    if(client_fd == -1){
        return 1;
    }

    printf("Got connection\n");
    ssize_t read_stat = 0;
    do{
        uint8_t bytes[128];

        fflush(stdout);
        read_stat = read(client_fd, bytes, sizeof(bytes) - 1);
        if( read_stat > 0 ){
            if(bytes[read_stat - 1] == '\n'){
                bytes[read_stat - 1] = 0;
            }
            bytes[read_stat] = 0;
            printf("Read %d bytes: %s\n", read_stat, bytes);
            lcc_gridconnect_incoming_data(gridconnect, bytes, read_stat);
        }else if(read_stat < 0){
            perror("read");
        }else{
            printf("Socket closed\n");
        }

    }while(read_stat > 0);

    return 0;
}
