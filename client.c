#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define LOG_FILE "client.log"

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

/**
 * Converts a log level enumeration  to a human-readable string representation.
 * 
 * @param level - the log level enum value to convert.
 * @return a pointer to a string that represents the log level.
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
 * Logs a message with a timestamp and the specified log level to a log file.
 * If the log file cannot be opened, an error message is printed to stderr.
 * 
 * @param level - the severity level of the message to log.
 * @param message - the message that needs to be logged.
*/
void log_message(LogLevel level, const char* message) {
    FILE* log_file = fopen(LOG_FILE, "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        char* time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline
        fprintf(log_file, "[%s] %s: %s\n", time_str, logLevelToString(level), message);
        fclose(log_file);
    } else {
        fprintf(stderr, "Failed to open log file.\n");
    }
}

/**
 * Establishes a connection to the server,
 * sends a greeting message, waits for a response, and then cleans up before exiting.
 * 
 * @return 0 on successful execution, and a non-zero value on errors.
*/
int main() {
    int sock;
    struct sockaddr_in server_address;
    char sendBuffer[BUFFER_SIZE] = "Hello, Server!";
    char recvBuffer[BUFFER_SIZE] = {0};

    log_message(LOG_INFO, "Starting client.");

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        log_message(LOG_ERROR, "Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    log_message(LOG_DEBUG, "Socket created successfully.");

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        log_message(LOG_ERROR, "Invalid address/ Address not supported.");
        close(sock);
        return -1;
    }
    log_message(LOG_DEBUG, "Server address set successfully.");

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection Failed");
        log_message(LOG_ERROR, "Connection to server failed.");
        close(sock);
        return -1;
    }
    log_message(LOG_INFO, "Connected to server successfully.");

    time_t time_now = time(NULL);
    snprintf(sendBuffer, BUFFER_SIZE, "Hello from client at %s", ctime(&time_now));

    ssize_t bytes_sent = send(sock, sendBuffer, strlen(sendBuffer), 0);
    if (bytes_sent < 0) {
        perror("Send failed");
        log_message(LOG_ERROR, "Failed to send message to server.");
        close(sock);
        exit(EXIT_FAILURE);
    }
    log_message(LOG_INFO, "Message sent to server successfully.");

    ssize_t bytes_received = recv(sock, recvBuffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror("Receive failed");
        log_message(LOG_ERROR, "Failed to receive reply from server.");
        close(sock);
        exit(EXIT_FAILURE);
    }
    recvBuffer[bytes_received] = '\0';
    printf("Server reply: %s\n", recvBuffer);
    log_message(LOG_INFO, "Received reply from server.");

    close(sock);
    log_message(LOG_INFO, "Connection closed. Client exiting.");

    return 0;
}
