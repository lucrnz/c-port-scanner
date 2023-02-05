#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define NUM_PARTS 20

typedef struct range {
    int start;
    int end;
} range;

struct scan_args {
    char *ip;
    range* ports;
    int timeout;
};

void *scan_port_thread(void *arg) {
    struct scan_args *args = (struct scan_args *)arg;
    int sock;
    struct sockaddr_in server;
    struct timeval tv;

    for (int port = args->ports->start; port < args->ports->end; port++) {
        // Create the socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("socket");
            exit(1);
        }

        // Set the timeout on the socket
        tv.tv_sec = args->timeout;
        tv.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("setsockopt");
            exit(1);
        }
        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            perror("setsockopt");
            exit(1);
        }
        // Prepare the sockaddr_in structure
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        if (inet_pton(AF_INET, args->ip, &server.sin_addr) <= 0) {
            perror("inet_pton");
            exit(1);
        }

        // Connect to the server
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
            printf("%d: closed\n", port);
        } else {
            printf("%d: open --- FOUND OPEN PORT ---\n", port);
        }

        // Close the socket
        close(sock);
    }
    

    return NULL;
}

struct range *divide_range(int start, int end) {
    int step = (end - start + NUM_PARTS - 1) / NUM_PARTS;
    struct range *ranges = malloc(NUM_PARTS * sizeof(struct range));

    int current_start = start;
    int current_end = start + step - 1;
    for (int i = 0; i < NUM_PARTS; i++) {
        ranges[i].start = current_start;
        ranges[i].end = current_end;
        current_start = current_end + 1;
        current_end = current_start + step - 1;
        if (current_end > end) {
            current_end = end;
        }
    }

    return ranges;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <ip address> <start_port> <end_port> <timeout>\n", argv[0]);
        return 1;
    }

    struct range *ranges = divide_range(atoi(argv[2]), atoi(argv[3]));

    pthread_t threads[NUM_PARTS];
    struct scan_args args[NUM_PARTS];


    // Read the command line arguments
    for (int i = 0; i < NUM_PARTS; i++) {
        args[i].ip = argv[1];
        args[i].ports = &ranges[i];
        args[i].timeout = atoi(argv[4]);
    }

    // Start the threads
    for (int i = 0; i < NUM_PARTS; i++) {
        int rc = pthread_create(&threads[i], NULL, scan_port_thread, &args[i]);
        if (rc) {
            fprintf(stderr, "Error creating thread %d\n", rc);
            exit(1);
        }
    }

    // Wait for the threads to finish
    for (int i = 0; i < NUM_PARTS; i++) {
        pthread_join(threads[i], NULL);
    }

    free(ranges);
    return 0;
}
