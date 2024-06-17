#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define PSEUDO_SIZE 24

int sockfd;

void handle_sigusr1(int sig) {
    write(sockfd, "Exit server.", strlen("Exit server."));
    exit(0);
}

void chat(int sockfd) {
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int n;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        n = select(sockfd + 1, &read_fds, NULL, NULL, NULL);
        if (n == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            bzero(buffer, BUFFER_SIZE);
            int bytes_read = read(sockfd, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) {
                printf("Server has closed the connection. Exiting...\n");
                break;
            }
            printf("%s", buffer);
            if (strncmp(buffer, "Server is shutting down", strlen("Server is shutting down")) == 0) {
                printf("Received shutdown message from server. Exiting...\n");
                break;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            bzero(buffer, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            write(sockfd, buffer, strlen(buffer));
            if (strncmp(buffer, "exit", 4) == 0) {
                printf("Client Exit...\n");
                break;
            }
        }
    }
}

int main() {
    struct sockaddr_in servaddr;

    signal(SIGUSR1, handle_sigusr1);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed...\n");
        exit(0);
    } else {
        printf("Socket successfully created..\n");
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Connection with the server failed...\n");
        exit(0);
    } else {
        printf("Connected to the server..\n");
    }

    printf("Client is running with PID %d\n", getpid());

    // Demande de pseudo
    char pseudo[PSEUDO_SIZE];
    printf("Enter your pseudo: ");
    fgets(pseudo, PSEUDO_SIZE, stdin);
    // Remove newline character from the pseudo
    size_t len = strlen(pseudo);
    if (len > 0 && pseudo[len - 1] == '\n') {
        pseudo[len - 1] = '\0';
    }

    // Envoi du pseudo au serveur
    write(sockfd, pseudo, strlen(pseudo) + 1);

    chat(sockfd);

    close(sockfd);
    return 0;
}