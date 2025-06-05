/*
 * Author: Abdallah Dib
 * Date: 2023-12-17
 * Description: Server program for a multi-threaded chat application using named pipes.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#define SERVER_PIPE_NAME "/tmp/ServerPipe"
#define MAX_CLIENTS 10

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
int connectedClientsCount = 0;
int exitFlag = 0;  // Flag to signal threads to exit

/*
 * Function: ClientHandler
 * ------------------------
 * Handles communication with a client.
 *
 * arg: a pointer to the client file descriptor
 *
 * returns: NULL
 */
void* ClientHandler(void* arg) {
    int clientFd = *(int*)arg;
    char buffer[256];

    // Lock the mutex before updating connectedClientsCount
    pthread_mutex_lock(&clientsMutex);
    connectedClientsCount++;
    pthread_mutex_unlock(&clientsMutex);

    // Release the lock before entering the loop
    pthread_mutex_unlock(&clientsMutex);

    while (1) {
        ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer));
        if (bytesRead != -1 && bytesRead != 0) {
            printf("%s\n", buffer);
        } else if (bytesRead == 0) {
            // Client disconnected
            pthread_mutex_lock(&clientsMutex);
            connectedClientsCount--;
            pthread_mutex_unlock(&clientsMutex);

            // No need to send a message to the server when a client disconnects
            break;
        }

        // Lock the mutex before checking the exitFlag
        pthread_mutex_lock(&clientsMutex);
        if (exitFlag) {
            pthread_mutex_unlock(&clientsMutex);
            break;
        }
        pthread_mutex_unlock(&clientsMutex);
    }

    close(clientFd);
    pthread_exit(NULL);
}

int main() {
    // Remove existing server named pipe
    if (unlink(SERVER_PIPE_NAME) == -1 && errno != ENOENT) {
        perror("Error removing existing server named pipe");
        exit(EXIT_FAILURE);
    }

    // Create the server named pipe
    if (mkfifo(SERVER_PIPE_NAME, 0666) == -1) {
        perror("Error creating server named pipe");
        exit(EXIT_FAILURE);
    }

    // Open the server named pipe
    int serverFd = open(SERVER_PIPE_NAME, O_RDONLY);
    if (serverFd == -1) {
        perror("Error opening server named pipe");
        exit(EXIT_FAILURE);
    }

    // Print a message indicating that the server has been initialized
    printf("Server initialized\n");

    // Main server loop
    while (1) {
        int clientFd = open(SERVER_PIPE_NAME, O_RDONLY);
        pthread_mutex_lock(&clientsMutex);

        if (connectedClientsCount < MAX_CLIENTS) {
            connectedClientsCount++;

            pthread_t tid;
            pthread_create(&tid, NULL, ClientHandler, &clientFd);
        } else {
            close(clientFd);
        }

        pthread_mutex_unlock(&clientsMutex);

        pthread_mutex_lock(&clientsMutex);
        if (exitFlag) {
            pthread_mutex_unlock(&clientsMutex);
            break;
        }
        pthread_mutex_unlock(&clientsMutex);
    }

    pthread_mutex_lock(&clientsMutex);
    exitFlag = 1;
    pthread_mutex_unlock(&clientsMutex);

    close(serverFd);

    return 0;
}
