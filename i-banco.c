/*
// Projeto SO - exercicio 4, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
// Realizado por:
// Pedro Salgueiro  83542
// Tiago Almeida    83568
*/

#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_TRANSFERIR "transferir"

#define OPERACAO_DEBITAR 1
#define OPERACAO_CREDITAR 2
#define OPERACAO_LER_SALDO 3
#define OPERACAO_SAIR 4
#define OPERACAO_TRANSFERIR 5

#define FALSE 0
#define TRUE 1

#define MAXFILHOS 20
#define MAXARGS 4
#define BUFFER_SIZE 100
#define NUM_TRABALHADORAS 3
#define PEDIDO_BUFFER_DIM (NUM_TRABALHADORAS * 2)

typedef struct {
  int operacao;
  int idConta;
  int idConta2;
  int valor;
} pedido_t;


void colocarPedido(pedido_t pedido);
pedido_t obterPedido();
void *threadFunc();

int posColocar = 0;
int posObter = 0;
int countBuffer = 0;

char filename[50];

pedido_t pedido_buffer[PEDIDO_BUFFER_DIM]; /* Tabela de pedidos */
pthread_t threads[NUM_TRABALHADORAS]; /* Tarefas trabalhadoras */
sem_t semObter, semColocar;
pthread_mutex_t mutex, mutexSimular;
pthread_cond_t podeSimular;

int main(int argc, char** argv) {

  char *args[MAXARGS + 1];
  char buffer[BUFFER_SIZE];
  int numFilhos = 0;
  int pidTable[MAXFILHOS];
  int posPid;
  int i; /* Iterar ciclos */

  pedido_t pedido;

  signal(SIGUSR1, tratarSignal);

  inicializarContas();

  /* Inicializar semaforos */
  if (sem_init(&semObter, 0, 0) != 0) {
    fprintf(stderr, "Erro a criar semaforo\n");
    exit(EXIT_FAILURE);
  }
  if (sem_init(&semColocar, 0, PEDIDO_BUFFER_DIM) != 0) {
    fprintf(stderr, "Erro a criar semaforo\n");
    exit(EXIT_FAILURE);
  }
  /* Inicializar threads */
  for (i = 0; i < NUM_TRABALHADORAS; i++)
    if (pthread_create(&threads[i], NULL, &threadFunc, NULL) != 0) {
      fprintf(stderr, "Erro a criar thread\n");
      exit(EXIT_FAILURE);
    }

  /* Inicializar mutexes i-banco.c */
  if(pthread_mutex_init(&mutex, NULL) != 0) {
  	fprintf(stderr, "Erro a inicializar mutex\n");
  	exit(EXIT_FAILURE);
  }
  if (pthread_mutex_init(&mutexSimular, NULL) != 0) {
  	fprintf(stderr, "Erro a inicializar mutex\n");
  	exit(EXIT_FAILURE);
  }

  /* Inicializar mutex contas.c */
  mutex_contas_init();

  /* Inicializar cond_t */
  if (pthread_cond_init(&podeSimular, NULL) != 0) {
    fprintf(stderr, "Erro a incializar cond");
    exit(EXIT_FAILURE);
  }

  abrir_ficheiro();
/* ========================================================================== */

  printf("Bem-vinda/o ao i-banco\n\n");

  while (1) {
    int numargs, status;

    numargs = readLineArguments(args, MAXARGS + 1, buffer, BUFFER_SIZE);

    /* EOF (end of file) do stdin ou comando "sair" */
    if (numargs < 0 ||
      (numargs > 0 && (strcmp(args[0], COMANDO_SAIR) == 0))) {
      int filho, pid;
      /* Sair normalmente */
      if (numargs == 1 || numargs < 0) {
        pedido_t pedido;
        pedido.operacao = OPERACAO_SAIR;
        for (i = 0; i < NUM_TRABALHADORAS; i++)
          colocarPedido(pedido);
        printf("i-banco vai terminar.\n--\n");
        for (filho = 0; filho < numFilhos; filho++) {
          pid = wait(&status);
          printf("FILHO TERMINADO (PID=%d; terminou %s)\n", pid,
            WIFSIGNALED(status) ? "abruptamente" : "normalmente");
        }
        for (i = 0; i < NUM_TRABALHADORAS; i++) {
          if (pthread_join(threads[i], NULL) != 0)
            fprintf(stderr, "Erro a sair da thread\n");
        }
      }

      /* Sair Agora */
      else if (numargs == 2 && strcmp(args[1], "agora") == 0) {
        pedido_t pedido;
        pedido.operacao = OPERACAO_SAIR;
        for(i = 0; i < NUM_TRABALHADORAS; i++)
          colocarPedido(pedido);
        printf("i-banco vai terminar.\n--\n");
        for (posPid = 0; posPid < numFilhos; posPid++) {
          kill(pidTable[posPid], SIGUSR1);
        }
        for (filho = 0; filho < numFilhos; filho++) {
          pid = wait(&status);
          printf("Simulacao terminada por signal\n");
          printf("FILHO TERMINADO (PID=%d; terminou %s)\n", pid,
            WIFSIGNALED(status) ? "abruptamente" : "normalmente");
        }
        for (i = 0; i < NUM_TRABALHADORAS; i++) {
          if (pthread_join(threads[i], NULL) != 0)
            fprintf(stderr, "Erro a sair da thread\n");
        }
      }

      else {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_SAIR);
        continue;
      }
      printf("--\ni-banco terminou.\n");
      exit(EXIT_SUCCESS);
    }

    else if (numargs == 0)
      /* Nenhum argumento; ignora e volta a pedir */
      continue;

    /* Debitar */
    else if (strcmp(args[0], COMANDO_DEBITAR) == 0) {

      if (numargs < 3) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_DEBITAR);
        continue;
      }

      pedido.operacao = OPERACAO_DEBITAR;
      pedido.idConta = atoi(args[1]);
      pedido.valor = atoi(args[2]);

      colocarPedido(pedido);
    }

    /* Creditar */
    else if (strcmp(args[0], COMANDO_CREDITAR) == 0) {

      if (numargs < 3) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_CREDITAR);
        continue;
      }

      pedido.operacao = OPERACAO_CREDITAR;
      pedido.idConta = atoi(args[1]);
      pedido.valor = atoi(args[2]);

      colocarPedido(pedido);
    }

    /* Transferir */
    else if (strcmp(args[0], COMANDO_TRANSFERIR) == 0) {

      if (numargs < 4 || atoi(args[1]) == atoi(args[2])) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_TRANSFERIR);
        continue;
      }

      pedido.operacao = OPERACAO_TRANSFERIR;
      pedido.idConta = atoi(args[1]);
      pedido.idConta2 = atoi(args[2]);
      pedido.valor = atoi(args[3]);

      colocarPedido(pedido);
    }

    /* Ler Saldo */
    else if (strcmp(args[0], COMANDO_LER_SALDO) == 0) {

      if (numargs < 2) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_LER_SALDO);
        continue;
      }

      pedido.operacao = OPERACAO_LER_SALDO;
      pedido.idConta = atoi(args[1]);
      pedido.valor = 0;

      colocarPedido(pedido);
    }

    /* Simular */
    else if (strcmp(args[0], COMANDO_SIMULAR) == 0) {
      int numAnos, pid, file_simular;

      if (numargs < 2) {
        printf("%s: Sintaxe inválida, tente de novo.\n", COMANDO_SIMULAR);
        continue;
      }
      numAnos = atoi(args[1]);

      if (numFilhos < MAXFILHOS){
        if(pthread_mutex_lock(&mutexSimular) != 0)
          fprintf(stderr, "Erro a fechar o mutex");
        while(countBuffer > 0) {
          pthread_cond_wait(&podeSimular, &mutexSimular);
        }
        pid = fork();

        if (pid == 0) {
          sprintf(filename, "i-banco-sim-%d.txt", getpid());
          file_simular = open(filename, O_CREAT | O_WRONLY, 0666);
          setFileDescriptor(-1);
          if(file_simular == -1) {
            fprintf(stderr, "Erro a abrir o ficheiro");
            close(file_simular);
            exit(-1);
          }
          if (dup2(file_simular, 1) == 0)
            fprintf(stderr, "Erro a redirecionar output\n");
          close(file_simular);
          simular(numAnos);
          exit(0);
        }
        else if (pid == -1) {
          fprintf(stderr, "Erro no fork\n");
          continue;
        }
        else {
          pidTable[numFilhos++] = pid;
          if(pthread_mutex_unlock(&mutexSimular) != 0)
            fprintf(stderr, "Erro a abrir mutex");
          continue;
        }
      }
    }

    /* Caso nenhum comando funcione */
    else {
      printf("Comando desconhecido. Tente de novo.\n");
      continue;
    }
  }
}

void colocarPedido(pedido_t pedido) {
  if(sem_wait(&semColocar) != 0) {
    fprintf(stderr, "Erro espera semaforo");
  	exit(EXIT_FAILURE);
  }
  if(pthread_mutex_lock(&mutex) != 0) {
    fprintf(stderr, "Erro fechar mutex");
    exit(EXIT_FAILURE);
  }
  pedido_buffer[posColocar] = pedido;
  posColocar = (posColocar + 1) % PEDIDO_BUFFER_DIM;
  countBuffer++;
  if(pthread_mutex_unlock(&mutex) != 0) {
    fprintf(stderr, "Erro abrir mutex");
    exit(EXIT_FAILURE);
  }
  if(sem_post(&semObter) != 0) {
    fprintf(stderr, "Erro assinalar semaforo");
    exit(EXIT_FAILURE);
  }
}

pedido_t obterPedido() {
  pedido_t pedido;
  if(sem_wait(&semObter) != 0) {
    fprintf(stderr, "Erro espera semaforo");
    exit(EXIT_FAILURE);
  }
  if(pthread_mutex_lock(&mutex) != 0) {
    fprintf(stderr, "Erro fechar mutex");
  	exit(EXIT_FAILURE);
  }
  pedido = pedido_buffer[posObter];
  posObter = (posObter + 1) % PEDIDO_BUFFER_DIM;
  if(pthread_mutex_unlock(&mutex) != 0) {
    fprintf(stderr, "Erro abrir mutex");
    exit(EXIT_FAILURE);
  }
  if(sem_post(&semColocar) != 0) {
    fprintf(stderr, "Erro assinalar semaforo");
    exit(EXIT_FAILURE);
  }

  /* Mudar variavel para poder simular */


  return pedido;
}

void *threadFunc() {

  while(TRUE) {
    pedido_t p = obterPedido();

    if (p.operacao == OPERACAO_DEBITAR) {
      debitar(p.idConta, p.valor);
    }

    else if (p.operacao == OPERACAO_CREDITAR) {
      creditar(p.idConta, p.valor);
   	}

    else if (p.operacao == OPERACAO_LER_SALDO) {
      lerSaldo(p.idConta);
    }

    else if (p.operacao == OPERACAO_TRANSFERIR) {
      transferir(p.idConta, p.idConta2, p.valor);
    }

    else if (p.operacao == OPERACAO_SAIR)
      return NULL;

    if(pthread_mutex_lock(&mutexSimular) != 0)
      fprintf(stderr, "Erro fechar mutex");
    countBuffer--;
    if(countBuffer == 0)
      if(pthread_cond_signal(&podeSimular) != 0)
      fprintf(stderr, "Erro a assinalar variavel podeSimular");
    if(pthread_mutex_unlock(&mutexSimular) != 0)
      fprintf(stderr, "Erro abrir mutex");
  }
}
