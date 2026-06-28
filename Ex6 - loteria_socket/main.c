/*
Escreva um programa C/C++ usando sockets para simular um sorteio da seguinte forma. 
Existem um processo pai e seus respectivos dez processos filhos. 
O processo pai aguarda por conexões TCP em uma porta arbitrária (você escolhe alguma porta a partir de 1024). 
Cada processo filho se conecta à porta do pai, informa seu PID e aguarda por uma mensagem informando o PID sorteado. 
Após receber todos os PIDs, o processo pai sorteia um PID entre aqueles informados, imprime na tela "PID sorteado: <pid>" , onde <pid> é o PID sorteado, e informa o PID sorteado aos fillhos através da conexão TCP anteriormente estabelecida. Os filhos recebem o PID sorteado e encerram a conexão TCP. Adicionalmente, o filho sorteado imprime na tela "<pid>: fui sorteado" onde pid é seu PID.

Instruções:

a) use o comando fork para criar processos filhos. Ou seja, não use threads.

b) use sockets de rede (AF_INET).

c) Se você estiver com problemas como "bind failed: Address already in use", siga a sugestão de https://stackoverflow.com/a/35419032/10923601 .
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define NPROCS 10
#define PORTA 1025

static void processo_filho(void) {
    pid_t meu_pid = getpid();
    pid_t pid_sorteado;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in servidor;
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PORTA);
    servidor.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&servidor, sizeof(servidor)) < 0) {
        perror("connect");
        close(fd);
        exit(1);
    }

    // informa PID pro pai
    if (write(fd, &meu_pid, sizeof(meu_pid)) != (ssize_t)sizeof(meu_pid)) {
        perror("write");
        close(fd);
        exit(1);
    }

    if (read(fd, &pid_sorteado, sizeof(pid_sorteado)) != (ssize_t)sizeof(pid_sorteado)) {
        perror("read");
        close(fd);
        exit(1);
    }

    if (pid_sorteado == meu_pid) {
        printf("%d: fui sorteado\n", meu_pid);
    }

    close(fd);
    exit(0);
}

int main(void) {
    int servidor_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_fd < 0) {
        perror("socket");
        return 1;
    }

    // alts abaixo para evitar o "address already in use" 
    int opt = 1;
    setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in endereco;
    memset(&endereco, 0, sizeof(endereco));
    endereco.sin_family = AF_INET;
    endereco.sin_port = htons(PORTA);
    endereco.sin_addr.s_addr = INADDR_ANY;

    if (bind(servidor_fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("bind");
        close(servidor_fd);
        return 1;
    }

    if (listen(servidor_fd, NPROCS) < 0) {
        perror("listen");
        close(servidor_fd);
        return 1;
    }

    // spawn dos 10 filhos
    for (int i = 0; i < NPROCS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(servidor_fd);
            return 1;
        }
        if (pid == 0) {
            close(servidor_fd); 
            processo_filho();
        }
    }

    // pai aceita as conex
    int conexoes[NPROCS];
    pid_t pids[NPROCS];

    for (int i = 0; i < NPROCS; i++) {
        int conn = accept(servidor_fd, NULL, NULL);
        if (conn < 0) {
            perror("accept");
            close(servidor_fd);
            return 1;
        }
        conexoes[i] = conn;

        if (read(conn, &pids[i], sizeof(pids[i])) != (ssize_t)sizeof(pids[i])) {
            perror("read");
            close(conn);
            close(servidor_fd);
            return 1;
        }
    }

    // 'sorteio'
    srand((unsigned int)time(NULL));
    pid_t pid_sorteado = pids[rand() % NPROCS];
    printf("PID sorteado: %d\n", pid_sorteado);


    for (int i = 0; i < NPROCS; i++) {
        if (write(conexoes[i], &pid_sorteado, sizeof(pid_sorteado)) != (ssize_t)sizeof(pid_sorteado)) {
            perror("write");
        }
        close(conexoes[i]);
    }

    close(servidor_fd);

    // wait nos filho
    for (int i = 0; i < NPROCS; i++) {
        wait(NULL);
    }

    return 0;
}
