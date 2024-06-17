#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define PSEUDO_SIZE 24
#define MAX_USER 2

typedef struct {
    int socket;
    char pseudo[PSEUDO_SIZE];
} User;

int client_count = 0;
User client_sockets[2] = {(User){-1, ""}, (User){-1, ""}};
int parent_pid;

void supprimer(int position) {
    if (position >= 0 && position < client_count) {
        client_sockets[client_count] = (User){-1, ""};
        client_count--;
    } else {
        printf("Position invalide. Impossible de supprimer.\n");
    }
}

void handle_sigusr1(int sig) {
    for (int i = 0; i < 2; i++) {
        if (client_sockets[i].socket != -1) {
            if (write(client_sockets[i].socket, "Server is shutting down. Closing connection.\n", 45) == -1) {
                perror("Error writing shutdown message");
            }
            close(client_sockets[i].socket);
        }
    }
    exit(0);
}

void handle_sigusr2(int sig) {
    printf("Current number of connections: %d\n", client_count);
    for (int i = 0; i < 2; i++) {
        printf("%d - Client name: %s - socket : %d\n", i, client_sockets[i].pseudo, client_sockets[i].socket);
    }
}

void handle_client(User client, User other_client) {
    char buffer[BUFFER_SIZE];
    User base = {-1, ""};

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        char message[BUFFER_SIZE + PSEUDO_SIZE + 3];
        bzero(message, sizeof(message));
        strcat(message, client.pseudo);
        strcat(message, " : ");

        int read_bytes = read(client.socket, buffer, BUFFER_SIZE);
        if (read_bytes <= 0) {
            printf("CTRL+C\n");
            // Déconnexion du client
            kill(parent_pid, SIGCHLD); // Notifier le parent
            if (other_client.socket != -1) {
                if (write(other_client.socket, "Waiting for another client to connect...\n", 42) == -1) {
                    perror("Error writing wait message");
                }
            }
            close(client.socket);
            exit(0);
        }

        if (strncmp(buffer, "Exit server.", strlen("Exit server.")) == 0) {
            printf("Exit\n");
            // Le client veut quitter le serveur
            kill(parent_pid, SIGCHLD); // Notifier le parent
            if (other_client.socket != -1) {
                if (write(other_client.socket, "Waiting for another client to connect...\n", 42) == -1) {
                    perror("Error writing wait message");
                }
            }
            close(client.socket);
            exit(0);
        }

        strcat(message, buffer);

        if (write(other_client.socket, message, strlen(message)) == -1) {
            perror("Error sending message");
        }
    }
}

void setup_user(int client_socket) {
    char buffer[PSEUDO_SIZE];
    bzero(buffer, PSEUDO_SIZE);

    // Lecture du pseudo envoyé par le client
    int read_bytes = read(client_socket, buffer, PSEUDO_SIZE);
    if (read_bytes > 1 && read_bytes < PSEUDO_SIZE) {
        buffer[read_bytes] = '\0';  // Ensure null-terminated string
    } else {
        strcpy(buffer, "Anonymous");
    }

    User u = {client_socket, ""};
    strncpy(u.pseudo, buffer, PSEUDO_SIZE - 1);

    if (client_count < MAX_USER) {
        for(int i = 0; i < MAX_USER; i++){
            if (client_sockets[i].socket == -1){
                client_sockets[i] = u;
                client_count++;
                break;
            }
        }
    } else {
        printf("Le tableau est plein. Impossible d'ajouter.\n");
    }

    printf("Client connected: %s\n", u.pseudo);
}

void update_client_list() {
    for (int i = 0; i < 2; i++) {
        if (client_sockets[i].socket != -1) {
            char buffer[1];
            int result = recv(client_sockets[i].socket, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
            if (result == 0) { // Client disconnected
                printf("Client %s disconnected.\n", client_sockets[i].pseudo);
                close(client_sockets[i].socket);
                client_sockets[i] = (User){-1, ""};
                client_count--;

            }
        }
    }
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    int server_socket, new_socket;
    socklen_t addr_len = sizeof(client_addr);

    parent_pid = getpid();

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    signal(SIGCHLD, update_client_list);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        perror("Socket bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 2) != 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is running with PID %d\n", getpid());
    printf("Server is listening on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (new_socket < 0) {
            perror("Server accept failed");
            continue;
        }

        setup_user(new_socket);

        if (client_count == 2) {
            // Notify both clients that they can start messaging
            if (write(client_sockets[0].socket, "You can start messaging now.\n", 30) == -1 ||
                write(client_sockets[1].socket, "You can start messaging now.\n", 30) == -1) {
                perror("Error notifying clients");
            }

            if (fork() == 0) {
                close(server_socket);
                handle_client(client_sockets[0], client_sockets[1]);
            }
            if (fork() == 0) {
                close(server_socket);
                handle_client(client_sockets[1], client_sockets[0]);
            }
        } else {
            // Inform the first client to wait for another client
            if (write(new_socket, "Waiting for another client to connect...\n", 42) == -1) {
                perror("Error writing wait message");
            }
        }
    }

    close(server_socket);
    return 0;
}