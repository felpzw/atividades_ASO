/*
Escreva um programa C/C++ usando pipe anônimo para simular um sorteio da seguinte forma.
Um processo pai cria 10 processos filhos e guarda os respectivos pids em um array.
Em seguida, o pai sorteia um pid e imprime no pipe "PID sorteado: <pid>", onde <pid> é o pid sorteado.
Os filhos leem o pipe, e um filho imprimirá "<pid>: fui sorteado" se o pid lido é equivalente seu pid.
Em seguida todos os processos encerram o pipe.

Instruções:

a) use o comando fork para criar processos filhos. Ou seja, não use threads.

b) o pai deverá escrever 10 vezes o pid sorteado no pipe, para que todos os filhos possam ler o pipe uma vez.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define NPROCS 10

static void processo_filho(int fd_leitura) {
    pid_t meu_pid = getpid();
    pid_t pid_sorteado;
    ssize_t lidos;

    lidos = read(fd_leitura, &pid_sorteado, sizeof(pid_sorteado));
    if (lidos != (ssize_t)sizeof(pid_sorteado)) {
        perror("read");
        close(fd_leitura);
        exit(1);
    }

    if (pid_sorteado == meu_pid) {
        printf("%d: fui sorteado\n", meu_pid);
    }

    close(fd_leitura);
    exit(0);
}

int main(void) {
    int pipefd[2];
    pid_t pids[NPROCS];
    pid_t pid_sorteado;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    for (int i = 0; i < NPROCS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            return 1;
        }

        if (pid == 0) {
            close(pipefd[1]);
            processo_filho(pipefd[0]);
        }

        pids[i] = pid;
    }

    close(pipefd[0]);

    srand((unsigned int)time(NULL));
    pid_sorteado = pids[rand() % NPROCS];
    printf("PID sorteado: %d\n", pid_sorteado);

    for (int i = 0; i < NPROCS; i++) {
        ssize_t escritos = write(pipefd[1], &pid_sorteado, sizeof(pid_sorteado));
        if (escritos != (ssize_t)sizeof(pid_sorteado)) {
            perror("write");
            close(pipefd[1]);
            return 1;
        }
    }

    close(pipefd[1]);

    for (int i = 0; i < NPROCS; i++) {
        wait(NULL);
    }

    return 0;
}