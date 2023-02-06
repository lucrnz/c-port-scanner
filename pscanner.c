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
#define MAX_PORTS 65535

pthread_mutex_t lock;

int* scanned_ports;

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
    assert(port > 0 && port < MAX_PORTS);
    assert(timeout >= 0);

    // Check if we already scanned this port
    pthread_mutex_lock(&lock);
    if (scanned_ports[port] != 0) {
        printf("Port %d already scanned\n", port);
        int value = scanned_ports[port];
        pthread_mutex_unlock(&lock);
        return value - 1;
    } else {
        pthread_mutex_unlock(&lock);
    }
    
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

    pthread_mutex_lock(&lock);
    scanned_ports[port] = result + 1;
    pthread_mutex_unlock(&lock);

    return result;
}

void *scan_port_thread(void *arg) {
    struct scan_args *args = (struct scan_args *)arg;

    assert(args->ip != NULL);
    assert(args->ports->end >= args->ports->start);
    assert(args->ports->start > 0 || args->ports->end < MAX_PORTS);
    assert(args->timeout >= 0);
    assert(args->ports != NULL);

    if (args->ports->start == args->ports->end) {
        printf("Scanning port %d\n", args->ports->start);
        if (scan_tcp_port(args->ip, args->ports->start, args->timeout)) {
            printf("Port %d -- OPEN\n", args->ports->start);
        }
        return NULL;
    }
    
    printf("Scanning ports %d to %d\n", args->ports->start, args->ports->end);
    for (int port = args->ports->start; port < args->ports->end; port++) {
        if (scan_tcp_port(args->ip, port, args->timeout)) {
            printf("Port %d -- OPEN\n", port);
        }
    }

    return NULL;
}

struct range *divide_range(int start, int end) {
    int step = (end - start + NUM_PARTS - 1) / NUM_PARTS;
    struct range *ranges = malloc(NUM_PARTS * sizeof(struct range));
    
    if (ranges == NULL) {
        perror("malloc");
        exit(1);
    }

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

    char* ip_address = argv[1];
    int port_range_start = atoi(argv[2]);
    int port_range_end = atoi(argv[3]);
    int timeout = atoi(argv[4]);

    // Validations
    if (ip_address == NULL) {
        fprintf(stderr, "Invalid IP address\n");
        return 1;
    }

    if (port_range_start > port_range_end || port_range_start == port_range_end) {
        fprintf(stderr, "Invalid port range\n");
        return 1;
    }

    if (port_range_start < 1 || port_range_end > MAX_PORTS) {
        fprintf(stderr, "Invalid port range\n");
        return 1;
    }

    if (timeout < 0) {
        fprintf(stderr, "Invalid timeout\n");
        return 1;
    }

    // Initialize the ports array
    scanned_ports = calloc(MAX_PORTS, sizeof(int));
    if (scanned_ports == NULL) {
        perror("calloc");
        exit(1);
    }

    // Initialize the mutex
    if (pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "Error initializing mutex\n");
        exit(1);
    }

    // Get ranges
    struct range *ranges = divide_range(port_range_start, port_range_end);

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

    // Destroy the mutex
    pthread_mutex_destroy(&lock);

    // Free the memory
    free(scanned_ports);
    free(ranges);

    return 0;
}
