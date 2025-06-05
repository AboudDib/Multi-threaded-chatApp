/*
 * Author: Abdallah Dib
 * Date: 2023-12-17
 * Description: Client program for a multi-threaded chat application using named pipes.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SERVER_PIPE_NAME "/tmp/ServerPipe"
#define AUTH_PASSWORD "password"

int main() {
    int serverFd;
    char username[50];
    char fullMessage[512];
    bool usernameJoined = false;

    while (1) {
        // Attempt to open the server named pipe
        while ((serverFd = open(SERVER_PIPE_NAME, O_WRONLY)) == -1) {
            // Server pipe does not exist or other error
            fprintf(stderr, "Server not available. Trying to reconnect...\n");
            sleep(1);
        }

        // Authenticate with the server using a password
        char password[50];
        printf("Enter the password: ");
        fgets(password, sizeof(password), stdin);
        if (strncmp(password, AUTH_PASSWORD, strlen(AUTH_PASSWORD)) != 0) {
            fprintf(stderr, "Incorrect password. Exiting.\n");
            close(serverFd);
            exit(EXIT_FAILURE);
        }

        while (1) {
            char message[256];

            if (!usernameJoined) {
                printf("Enter your username: ");
                fgets(username, sizeof(username), stdin);

                size_t len = strlen(username);
                if (len > 0 && username[len - 1] == '\n') {
                    username[len - 1] = '\0';
                }

                // Notify the server that the user has joined
                snprintf(fullMessage, sizeof(fullMessage), "%s joined", username);
                if (write(serverFd, fullMessage, strlen(fullMessage) + 1) == -1) {
                    perror("Error writing to the server pipe");
                    close(serverFd);
                    exit(EXIT_FAILURE);
                }

                usernameJoined = true;
            } else {
                printf("Enter message (type 'exit' to quit): ");
                fgets(message, sizeof(message), stdin);

                // Check for server disconnection
                if (serverFd == -1) {
                    fprintf(stderr, "Server disconnected. Trying to reconnect...\n");
                    break;
                }

                // Check for user exit command
                if (strncmp(message, "exit", sizeof("exit") - 1) == 0) {
                    // Notify the server that the user has exited
                    snprintf(fullMessage, sizeof(fullMessage), "%s exited", username);
                    if (write(serverFd, fullMessage, strlen(fullMessage) + 1) == -1) {
                        perror("Error writing to the server pipe");
                        close(serverFd);
                        exit(EXIT_FAILURE);
                    }

                    // Terminate the client
                    fprintf(stderr, "Exiting the client.\n");
                    close(serverFd);
                    exit(EXIT_SUCCESS);
                }

                // Send regular messages to the server
                snprintf(fullMessage, sizeof(fullMessage), "%s: %s", username, message);
                if (write(serverFd, fullMessage, strlen(fullMessage) + 1) == -1) {
                    perror("Error writing to the server pipe");
                    close(serverFd);
                    exit(EXIT_FAILURE);
                }
            }
        }

        close(serverFd);
    }

    return 0;
}
