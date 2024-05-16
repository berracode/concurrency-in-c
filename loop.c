#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8079
#define MAX_CLIENTS 100
#define TIMEOUT 3

// Función para manejar eventos de temporizador
void handle_timer_event() {
    printf("Se ha producido un evento de temporizador.\n");
}

// Función para manejar eventos de entrada en el socket
void handle_socket_event(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Nueva conexión aceptada, socket fd: %d\n", client_socket);
    // Enviar un mensaje HTTP al cliente
    const char *message = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Hola Cliente\n";
    send(client_socket, message, strlen(message), 0);

    close(client_socket);

}

// Función para iniciar el servidor y obtener el descriptor de archivo del socket del servidor
int init_server() {
    int server_fd;
    struct sockaddr_in server_addr;

    // Crear el socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Vincular el socket a la dirección y el puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Escuchar en el socket
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d\n", PORT);

    return server_fd;
}

// Función para esperar eventos y ejecutar el bucle de eventos
void event_loop(int server_fd) {
    fd_set read_fds;
    struct timeval timeout;

    while (1) {
        // Configurar el conjunto de descriptores de archivo para el socket del servidor
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        // Configurar el temporizador para el tiempo de espera
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        // Esperar eventos en el socket del servidor o esperar hasta que se alcance el tiempo de espera
        int ready = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready == 0) {
            // Se ha alcanzado el tiempo de espera
            handle_timer_event();
        } else {
            // Se ha recibido una conexión entrante en el socket del servidor
            if (FD_ISSET(server_fd, &read_fds)) {
                handle_socket_event(server_fd);
            }
        }
    }
}

int main() {
    int server_fd = init_server();

    printf("Iniciando el bucle de eventos...\n");
    event_loop(server_fd);

    // Cerrar el socket del servidor
    close(server_fd);

    return 0;
}
