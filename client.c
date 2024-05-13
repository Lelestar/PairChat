#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    while (1) {
        printf("Donnez votre réponse: ");
        char message[1024];
        fgets(message, 1024, stdin);

        send(sock, message, strlen(message), 0);
        printf("Message envoyé\n");

        valread = read(sock, buffer, 1024);
        if (valread > 0) {
            printf("Le client distant dit: %s\n", buffer);
        } else {
            printf("Le serveur a fermé la connexion\n");
            break;
        }
        memset(buffer, 0, sizeof(buffer));  // Nettoyer le buffer après chaque lecture
    }

    close(sock);
    return 0;
}
