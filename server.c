#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define LOG_FILE "server.log"

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

/**
 * Converts a log level enumeration to a human-readable string.
 * 
 * @param level - the log level to conver.
 * @return A string representing the log level.
*/
const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

/**
 * Logs a message to the server log file with a timestamp and the log level
 * 
 * @param level - the log level of the message.
 * @param message - the message to log.
*/
void log_message(LogLevel level, const char* message) {
    FILE* log_file = fopen(LOG_FILE, "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(log_file, "[%s] %s: %s\n", time_str, logLevelToString(level), message);
        fclose(log_file);
    } else {
        fprintf(stderr, "Failed to open log file.\n");
    }
}

int server_fd;

/**
 * Handles SIGINT and SIGTERM signals to gracefully shut down the server.
 * Closes the server socket and exits the program.
 * 
 * @param sig - the signal number received
*/
void signal_handler(int sig) {
    (void)sig;
    printf("Shutdown signal received\n");
    log_message(LOG_INFO, "Shutdown signal received, closing server.");
    if (server_fd >= 0) {
        close(server_fd);
    }
    exit(0);
}

/**
 * Sets up the signal handling for SIGINT and SIGTERM using sigaction.
 * Ensures the server can be gracefully shut down on these signals.
*/
void setup_signal_handling() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/**
 * Handles client connections by reading a message from the client,
 * responding to the client, and then closing the connection.
 * This function is designed to be run in a separate thread for each client.
 * 
 * @param socket_desc - a pointer to the socket descriptor for the client connection.
 * @return NULL.
*/
void *handle_client(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE] = {0};
    char *message = "Hello from the server";

    ssize_t bytes_read = read(sock, buffer, BUFFER_SIZE);
    if(bytes_read < 0) {
        perror("Read");
        log_message(LOG_ERROR, "Failed to read client message.");
        free(socket_desc);
        close(sock);
        return NULL;
    }
    log_message(LOG_INFO, "Successfully read message from client.");
    printf("Message from client: %s\n", buffer);

    ssize_t bytes_sent = send(sock, message, strlen(message), 0);
    if (bytes_sent < 0) {
        perror("Send");
        log_message(LOG_ERROR, "Failed to send message to client.");
        free(socket_desc);
        close(sock);
        return NULL;
    }
    log_message(LOG_INFO, "Successfully sent message to client.");
    printf("Hello message sent\n");

    close(sock);
    free(socket_desc);
    log_message(LOG_INFO, "Closed the socket connection and freed the descriptor.");

    return NULL;
}

/**
 * Initializes the server, sets up signal handling, then enters a loop
 * to accept and handle client connections in separate threads.
*/
int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    log_message(LOG_INFO, "Server starting.");
    setup_signal_handling();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("SOCKET FAILED");
        log_message(LOG_ERROR, "Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("BIND FAILED");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("LISTEN");
        exit(EXIT_FAILURE);
    }

    while(1) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("malloc");
            close(new_socket);
            continue;
        }
        *new_sock = new_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            perror("could not create thread");
            free(new_sock);
            close(new_socket);
            continue;
        }

        pthread_detach(thread_id);
    }

    return 0;
}
