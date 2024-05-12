#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define NUM_THREADS 2
#define QUEUE_SIZE 3

#define PORT 8080
#define MAX_CLIENTS 100

typedef struct {
    void (*task_function)(void *); // pointer to task function
    void *arg; // arg to task function
} Task;

// simple ThreadPool struct
typedef struct {
    pthread_t *threads; // thread's array
    int num_threads; // numero de hilos
    Task queue[QUEUE_SIZE]; // task array
    int head; // cabeza de la cola
    int tail; // final de la cola
    pthread_mutex_t mutex; // Mutex para garantizar la exclusión mutua en el acceso a la cola
    pthread_cond_t cond; // Condición para la espera de nuevas tareas
    bool stop; // Bandera para indicar si se debe detener el pool
    int queue_capacity;
} ThreadPool;

ThreadPool *pool;
int server_fd, mutex_lock;

void *worker_function(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    printf("POol tail in worker: %d | HILO: %lu\n", pool->tail, pthread_self());
    while (true) {
        printf("Antes del mutex %lu\n", pthread_self());
        pthread_mutex_lock(&(pool->mutex));
        printf("pool->head [%d] == pool->tail [%d] && !pool->stop [%d]\n", pool->head , pool->tail, !pool->stop);
        while (pool->head == pool->tail && !pool->stop) {
            printf("New capacity: %d\n", pool->queue_capacity);
            printf("Esperando...\n");
            pthread_cond_wait(&(pool->cond), &(pool->mutex));
        }
        printf("despues del while de cond_wait\n");
        if (pool->stop) {
            pthread_mutex_unlock(&(pool->mutex));
            pthread_exit(NULL);
        }
        printf("Antes del task\n");
        printf("#### pool->head [%d] == pool->tail [%d] && !pool->stop [%d]\n", pool->head , pool->tail, !pool->stop);
        Task task = pool->queue[pool->head];
        printf("DESNCOLANDO: %p\n", &(pool->queue[pool->head]));
        pool->head = (pool->head + 1) % QUEUE_SIZE;
        pthread_mutex_unlock(&(pool->mutex));

        // Ejecutar la tarea
        (*(task.task_function))(task.arg);
        

    }
    return NULL;
}

// Función para crear el pool de hilos
ThreadPool *threadpool_create(int num_threads) {
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    pool->num_threads = num_threads;
    pool->head = 0;
    pool->tail = 0;
    pool->stop = false;
    pool->queue_capacity = QUEUE_SIZE;
    pthread_mutex_init(&(pool->mutex), NULL);
    pthread_cond_init(&(pool->cond), NULL);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&(pool->threads[i]), NULL, worker_function, pool);
    }
    return pool;
}

// Función para agregar una tarea al pool
void threadpool_add_task(ThreadPool *pool, void (*task_function)(void *), void *arg) {
    pthread_mutex_lock(&(pool->mutex));

    printf("pool->head [%d]\n", pool->head);
    printf("pool->tail [%d]\n", pool->tail);

    pool->queue[pool->tail].task_function = task_function;
    pool->queue[pool->tail].arg = arg;

    printf("queue[%d] tiene a %p\n",pool->tail, &(pool->queue[pool->tail]));

    pool->tail = (pool->tail + 1) % QUEUE_SIZE;
    pool->queue_capacity = (pool->queue_capacity - 1) % QUEUE_SIZE;
    printf("## QUEUE CAPACITY: %d\n", pool->queue_capacity);
    printf("## TAIL %d\n", pool->tail);

    pthread_mutex_unlock(&(pool->mutex));
    pthread_cond_signal(&(pool->cond));
}

// Función para destruir el pool de hilos
void threadpool_destroy(ThreadPool *pool) {
    printf("LIberando pool...\n");
    pthread_mutex_lock(&(pool->mutex));
    pool->stop = true;
    pthread_mutex_unlock(&(pool->mutex));
    pthread_cond_broadcast(&(pool->cond));
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    free(pool->threads);
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->cond));
    free(pool);
}
// Función que maneja una conexión entrante
void handle_connection(void *arg) {
    printf("Thread ID: %lu\n", pthread_self());
    int client_socket = *((int* )arg);
    // Procesar la conexión
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if (getpeername(client_socket, (struct sockaddr *)&client_addr, &client_addr_len) != 0) {
        perror("getpeername failed");
        close(client_socket);
        return;
    }

    // Procesar la conexión
    printf("Handling connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Enviar un mensaje HTTP al cliente
    const char *message = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Hola Cliente\n";
    send(client_socket, message, strlen(message), 0);

    // Cerrar el socket del cliente
    close(client_socket);
    printf("Cliente cerrado\n");
}


void sigint_handler(int sig) {
    printf("\nReceived SIGINT signal. Cleaning up...\n");
    
    // Detener nuevas conexiones
    shutdown(server_fd, SHUT_RDWR);
    
    // Liberar el threadpool
    threadpool_destroy(pool);

    // Cerrar el socket del servidor
    close(server_fd);
    
    printf("Cleanup complete. Exiting.\n");
    exit(EXIT_SUCCESS);
}


int main() {

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting signal handler");
        return EXIT_FAILURE;
    }

   int client_socket;
    struct sockaddr_in server_address, client_address;
    int address_len = sizeof(server_address);

    // Crear el socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Vincular el socket a la dirección y el puerto
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuchar en el socket
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    // Crear el threadpool
    pool = threadpool_create(NUM_THREADS);


// Aceptar conexiones entrantes y manejarlas en el pool de hilos
    while (1) {
        // Aceptar una conexión entrante
        if ((client_socket = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        printf("--------------------------------\n\n");
        printf("New connection, socket fd is %d,  %s:%d\n",
               client_socket, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Agregar la tarea de manejar la conexión al pool de hilos
        threadpool_add_task(pool, (void (*)(void *))handle_connection, &client_socket);

        printf("--------------------------------\n\n");

    }


    return 0;
}
