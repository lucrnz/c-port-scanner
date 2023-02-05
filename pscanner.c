#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>

#define NUM_PARTS 20

// Todo: Make an array and store the results in it.
// This way we can check if we already scanned a port or not.
// int** scanned_ports;

typedef struct range {
    int start;
    int end;
} range;

struct scan_args {
    char *ip;
    range* ports;
    int timeout;
};

int scan_tcp_port(char *ip, int port, int timeout) {
    assert(ip != NULL);
    assert(port > 0 && port < 65535);
    assert(timeout >= 0);

    int result = 0;
    int sock;
    struct sockaddr_in server;
    struct timeval tv;

    // Create the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    // Set the timeout on the socket
    tv.tv_sec = timeout;
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
    if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        result = 0;
    } else {
        result = 1;
    }

    // Close the socket
    close(sock);

    return result;
}

void *scan_port_thread(void *arg) {
    struct scan_args *args = (struct scan_args *)arg;

    assert(args->ip != NULL);
    assert(args->ports->end >= args->ports->start);
    assert(args->ports->start > 0 || args->ports->end <= 65535);
    assert(args->timeout >= 0);
    assert(args->ports != NULL);

    if (args->ports->start == args->ports->end) {
        printf("Scanning port %d\n", args->ports->start);
        if (scan_tcp_port(args->ip, args->ports->start, args->timeout)) {
            printf("Port %d -- OPEN\n", args->ports->start);
        }
    } else {
        printf("Scanning ports %d to %d\n", args->ports->start, args->ports->end);
        for (int port = args->ports->start; port < args->ports->end; port++) {
            if (scan_tcp_port(args->ip, port, args->timeout)) {
                printf("Port %d -- OPEN\n", port);
            }
        }
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
    // Usage
    if (argc != 5) {
        printf("Usage: %s <ip address> <start_port> <end_port> <timeout>\n", argv[0]);
        return 1;
    }

    // Validations
    if (atoi(argv[2]) > atoi(argv[3])) {
        fprintf(stderr, "Invalid port range\n");
        return 1;
    }

    if (atoi(argv[2]) < 1 || atoi(argv[3]) > 65535) {
        fprintf(stderr, "Invalid port range\n");
        return 1;
    }

    if (atoi(argv[4]) < 0) {
        fprintf(stderr, "Invalid timeout\n");
        return 1;
    }

    // Get ranges
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
