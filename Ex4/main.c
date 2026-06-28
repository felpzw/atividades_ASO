/*

Escreva um programa em C/C++ que lê do teclado (stdin) a quantidade de processos a serem criados (dica: use std::cin >> nprocs, onde nprocs > 0 é uma variável inteira). 
O processo principal (processo pai) cria procs processos filhos com uso da chamada de sistema fork(). 
Cada processo filho deverá ficar bloqueado durante um tempo aleatório entre 1s e 10s e deverá imprimir "Processo XX criado", onde XX é um identificador único de 1 até nprocs. Em outras palavras, XX deverá ser 1 para o primeiro filho que acordar, 2 para o segundo filho, 3 para o terceiro filho, etc...

Para realizar essa tarefa, você precisará: 
(i) criar uma região de memória compartilhada entre os processos(uma variável inteira denominada aqui de "id" e inicializada com valor 0); 
(ii) inicializar um semáforo compartilhado entre todos os processos que será utilizado como um mecanismo de exclusão mútua; 
(iii) fazer com que cada processo filho, entre em uma região crítica, incremente id, imprima "Processo <id> criado", 
onde <id> é o valor da variável id, e saia da região crítica.

Dica: para a criação de uma região de memória compartilhada entre processos, pesquise sobre a chamada de sistema mmap() ou shmget(). 
Para a criação de um semáforo compartilhado entre processos, pesquise sobre as funções sem_open() e sem_unlink() disponivel em <semaphore.h>

Aviso: não use as chamadas semget e semop disponível em <sys/sem.h>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>


#define tempo_aleatorio() (rand() % 10 + 1) //ENTRE 1 E 10 SEGS


void criar_processos(int nprocs) {
    int *id = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    //int *id = malloc(sizeof(int));
    *id = 0; 
    sem_t *sem = sem_open("/semaforo", O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        printf("Erro na criacao do semaforo\n");
        exit(1);
    }

    for (int i = 0; i < nprocs; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            printf("Ocorreu algum erro\n");
            exit(1);
        } else if (pid == 0) {
            srand(time(NULL) ^ getpid()); //semente por filho p/ tempos distintos
            sleep(tempo_aleatorio());

            sem_wait(sem);
            (*id)++; //ponteiro do id avanc 1 casa INTEIRA para o prox id
            printf("Processo %d criado\n", *id);
            sem_post(sem);

            exit(0);
        }
    }

    // For milagroso do p_id_t para aguardar todos os processos terminarem 
    for (int i = 0; i < nprocs; i++) {
        wait(NULL);
    }

    sem_close(sem);
    sem_unlink("/semaforo");
}

int main() {
    int nprocs;
    printf("Digite a quantidade de processos a serem criados: ");
    fflush(stdout); //esvazia o buffer antes do fork p/ nao duplicar o prompt
    scanf("%d", &nprocs);

    if (nprocs <= 0) {
        fprintf(stderr, "A quantidade de processos deve ser maior que 0.\n");
        exit(1);
    }

    criar_processos(nprocs);
    return 0;
}




