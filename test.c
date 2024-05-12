#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 20

int shared_variable = 0; // Variable compartida entre los hilos
pthread_mutex_t mutex; // Mutex para garantizar la exclusión mutua en el acceso a la variable compartida

// Función que será ejecutada por los hilos
void *thread_function(void *arg) {
    int thread_id = *(int *)arg;
    for (int i = 0; i < 20000; i++) {
        // Bloquear el mutex antes de acceder a la variable compartida
        pthread_mutex_lock(&mutex);
        // Acceder a la variable compartida
        shared_variable++;
        printf("Thread %d: Shared variable = %d\n", thread_id, shared_variable);
        // Desbloquear el mutex después de acceder a la variable compartida
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS] = {1, 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

    // Inicializar el mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    // Crear los hilos
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, thread_function, &i) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Thread join failed");
            exit(EXIT_FAILURE);
        }
    }

    // Destruir el mutex después de su uso
    pthread_mutex_destroy(&mutex);

    return 0;
}
