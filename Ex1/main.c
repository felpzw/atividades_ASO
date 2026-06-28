/*
Escreva um programa em C onde a thread principal cria outras 128 threads. Deve existir um contador global iniciado em 0. Cada uma das 128 threads deve incrementar o contador 1000 vezes. A thread principal deve aguardar as demais threads terminarem e então imprimir o valor final do contador.

Claramente, existe uma condição de corrida. Elimine-a utilizando um mutex.

Você deve utilizar a biblioteca pthreads.
*/
#include <stdio.h>
#include <pthread.h>

int contador = 0;
pthread_mutex_t mutex;

int th_number = 128;

void* thread_function(void *args) {
    for (int i = 0; i < 1000; i++) {
        pthread_mutex_lock(&mutex);
        contador++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_mutex_init(&mutex, NULL);
    pthread_t threads[th_number];

    for (int i = 0; i < th_number; i++) {
        pthread_create(&threads[i], NULL, thread_function, NULL);
    }

    for (int i = 0; i < th_number; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Valor final do contador: %d\n", contador);
    pthread_mutex_destroy(&mutex);
    return 0;
}