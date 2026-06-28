#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 5

typedef struct {
    char estados[N];
    pthread_mutex_t mutex;
    pthread_cond_t cond[N];
} Mesa;

typedef struct {
    int id;
    Mesa *mesa; // todo mundo sabe o estado atual da mesa, e o proprio id
} Filosofo;

//(id + N - 1) % N      FUNCAO PARA PEGAR VALOR A 'ESQUERDA'
//(id + 1) % N          FUNCAO PARA PEGAR VALOR A 'DIREITA'

// prototipo das 'traits'
void Mesa_imprimir(Mesa *self);
void Mesa_testar(Mesa *self, int id);
void pegar_garfos(Filosofo *self);
void devolver_garfos(Filosofo *self);
void* filosofo_rotina(void* arg);

// funcoes da mesa de filosofos

void Mesa_imprimir(Mesa *self) {
    for (int i = 0; i < N; i++) {
        printf("%c", self->estados[i]);
    }
    printf("\n");
}

void Mesa_testar(Mesa *self, int id) {
    int esquerda = (id + N - 1) % N;
    int direita = (id + 1) % N;

    if (self->estados[id] == 'F' && 
        self->estados[esquerda] != 'C' && 
        self->estados[direita] != 'C') {
        
        self->estados[id] = 'C';
        printf("Filosofo %d conseguiu os garfos e vai comer\n", id); //debug
        Mesa_imprimir(self);
        pthread_cond_signal(&self->cond[id]); //emit sinal de para o id
    }
}

//Funcoes dos filosofos

//func consumidora do sinal de condicao
void pegar_garfos(Filosofo *self) {
    pthread_mutex_lock(&self->mesa->mutex);

    self->mesa->estados[self->id] = 'F';
    Mesa_imprimir(self->mesa);
    //printf("Filosofo %d FICOU faminto e vai tentar garfos\n", self->id); //debug
    
    Mesa_testar(self->mesa, self->id);

    while (self->mesa->estados[self->id] != 'C') {
        //printf("Filosofo %d bloqueado... aguardando vizinhos.\n", self->id);//debug
        pthread_cond_wait(&self->mesa->cond[self->id], &self->mesa->mutex);

    }

    printf("Filosofo %d recebeu o sinal e saiu do loop de aguardo\n", self->id); //debug

    pthread_mutex_unlock(&self->mesa->mutex);
}

void devolver_garfos(Filosofo *self) {
    pthread_mutex_lock(&self->mesa->mutex);

    self->mesa->estados[self->id] = 'P';
    Mesa_imprimir(self->mesa);

    // ultiliza a funcao para tentar emitir o sinal de estado para os vizinhos
    Mesa_testar(self->mesa, (self->id + N - 1) % N);
    Mesa_testar(self->mesa, (self->id + 1) % N);

    pthread_mutex_unlock(&self->mesa->mutex);
}

void* filosofo_rotina(void* arg) {
    Filosofo* self = (Filosofo*)arg;
    
        sleep(rand() % 3 + 1);
        pegar_garfos(self);
        sleep(rand() % 3 + 1);
        devolver_garfos(self);
    return NULL;
}

//FIM DAS FUNCOES DE CLASSE

int main() {
    Mesa mesa_global;
    Filosofo filosofos[N];
    pthread_t threads[N];

    pthread_mutex_init(&mesa_global.mutex, NULL);
    for (int i = 0; i < N; i++) {
        mesa_global.estados[i] = 'P';
        pthread_cond_init(&mesa_global.cond[i], NULL);
    }

    Mesa_imprimir(&mesa_global);

    for (int i = 0; i < N; i++) {
        filosofos[i].id = i;
        filosofos[i].mesa = &mesa_global;
        pthread_create(&threads[i], NULL, filosofo_rotina, &filosofos[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}