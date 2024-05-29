#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define PORT 8080
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

int server_fd;  // Déclaration globale pour faciliter la gestion des signaux
int client_sockets[MAX_CLIENTS];
int active_connections = 0;

void handle_sigusr1(int sig) {
    printf("Fermeture forcée par SIGUSR1\n");
    for (int i = 0; i < active_connections; i++) {
        close(client_sockets[i]);  // Fermer tous les sockets clients
    }
    close(server_fd);  // Fermer le socket serveur
    exit(0);
}

void handle_sigusr2(int sig) {
    printf("Nombre de connexions actives : %d\n", active_connections);
}

void child_terminated(int sig) {
    while(waitpid(-1, NULL, WNOHANG) > 0){ // Gérer la suppression d'un client dans le nombre de connexions actives
        active_connections--;
    }
}



int main() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    signal(SIGCHLD, child_terminated);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Afficher le PID du processus serveur
    printf("Serveur démarré avec PID : %d\n", getpid());

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
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if (active_connections < MAX_CLIENTS) {
            client_sockets[active_connections++] = new_socket;  // Ajouter le nouveau socket client
            int client_id = active_connections - 1;  // Identifiant du client (index dans le tableau)

            printf("Nouvelle connexion acceptée. Client (Id: %d). Nombre de clients connectés : %d\n", client_id, active_connections);

            if (fork() == 0) {
                // Dans le processus fils
                close(server_fd);  // Fermer le socket serveur copié dans le fils

                char buffer[BUFFER_SIZE];
                while (1) {
                    memset(buffer, 0, sizeof(buffer));
                    int bytes_received = recv(client_sockets[client_id], buffer, sizeof(buffer), 0);
                    if (bytes_received <= 0) {
                        // Erreur de réception ou client déconnecté
                        printf("Client%d déconnecté.\n", client_id);
                        break;
                    }

                    // Afficher le message reçu avec l'identifiant du client
                    printf("Client%d : %s\n", client_id, buffer);

                    // Diffuser le message reçu à tous les autres clients
                    for (int i = 0; i < active_connections; i++) {
                        if (i != client_id) {
                            send(client_sockets[i], buffer, strlen(buffer), 0);
                        }
                    }
                }

                // Fermer le socket du client et terminer le processus fils
                close(client_sockets[client_id]);
                exit(0);
            }
        } else {
            printf("Nombre maximal de connexions atteint. Rejet du client.\n");
            close(new_socket);  // Fermer la connexion du client car le nombre maximal est atteint
        }
    }

    return 0;
}
