#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>

#define PORT 8080

int server_fd;  // Déclaration globale pour faciliter la gestion des signaux
int active_connections = 0;

void handle_sigusr1(int sig) {
    printf("Fermeture forcée par SIGUSR1\n");
    close(server_fd);  // Fermer le socket serveur
    exit(0);
}

void handle_sigusr2(int sig) {
    printf("Nombre de connexions actives : %d\n", active_connections);
}

int main() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        active_connections++;  // Incrémenter à chaque connexion acceptée
        if (fork() == 0) {
            // Dans le processus fils
            close(server_fd);  // Fermer le socket serveur copié dans le fils, pas nécessaire ici
            // Commencez à gérer la communication avec new_socket
            // Exemple de gestion simplifiée:
            char buffer[1024] = {0};
            while (read(new_socket, buffer, 1024) > 0) {
                printf("Client dit: %s\n", buffer);
                send(new_socket, buffer, strlen(buffer), 0);  // Echo back
                memset(buffer, 0, 1024);
            }
            close(new_socket);
            exit(0);
        } else {
            // Dans le processus parent
            close(new_socket);  // Important de fermer la nouvelle socket dans le parent
        }
    }

    return 0;
}
