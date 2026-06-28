/*
Escreva um programa que leia um número n>1 do teclado (veja a função scanf) e escreva 1 se o n for primo e 0 caso contrário.
Para a verificação, utilize duas threads onde cada thread verifica metade dos possíveis fatores de n. 
A thread que encontrar um divisor (neste caso, o número não é primo) deve cancelar a outra thread e terminar Para cancelar a outra thread, 
utilize a função pthread_cancel (ver link) e pthread_canceltype (ver link).

O código C para leitura do teclado e escrita do resultado na tela já está disponível e você não deve modificá-lo. 
Lembre de avaliar seu código clicando no botão "avaliar" (botão com checkbox seguido de um zero).
*/

#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex;

typedef struct {
    long n;
    long start;
    long end;
    pthread_t *other_thread; 
    //int *is_prime;           
} ThreadArgs;

int global_is_prime = 1;



void* thread_function(void* arg) {
    ThreadArgs* data = (ThreadArgs*)arg;
    
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    for (long i = data->start; i <= data->end; i++) {
        if (data->n % i == 0) {
            pthread_mutex_lock(&mutex);
            //*(data->is_prime) = 0;
            global_is_prime = 0;
            pthread_mutex_unlock(&mutex);
            
            pthread_cancel(*(data->other_thread));
            return NULL;
        }
    }
    return NULL;
}

int main(){
    long x;
    scanf("%ld",&x);

    if (x <= 1) {
        printf("0\n");
        return 0;
    }

    //separar o valor de x
    long start1 =  2;
    long end1 = x / 2;
    long start2 = x / 2 + 1;
    long end2 = x - 1;



    pthread_t thread1 = 0, thread2 = 0;

    pthread_mutex_init(&mutex, NULL);

    ThreadArgs args1 = {x, 2, end1, &thread2, &global_is_prime};
    ThreadArgs args2 = {x, start2, end2, &thread1, &global_is_prime};

    pthread_mutex_lock(&mutex);

    pthread_create(&thread1, NULL, thread_function, &args1);
    pthread_create(&thread2, NULL, thread_function, &args2);

    pthread_mutex_unlock(&mutex);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    pthread_mutex_destroy(&mutex);

    if (global_is_prime) {
        printf("O valor %ld é primo\n", x);
    } else {
        printf("O valor %ld não é primo\n", x);
    }


    return global_is_prime;
    }