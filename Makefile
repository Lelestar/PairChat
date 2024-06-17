# Makefile

# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -O2

# Cibles
TARGETS = MessagerieServer MessagerieClient

# Dépendances
all: $(TARGETS)

MessagerieServer: server.c
	$(CC) $(CFLAGS) -o $@ $^

MessagerieClient: client.c
	$(CC) $(CFLAGS) -o $@ $^

# Nettoyage des fichiers objets et des exécutables
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
