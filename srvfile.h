#ifndef SRVFILE_H
#define SRVFILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Macro para tratar erros
#define exit_on_error(s,m) if (s < 0) { perror(m); exit(1); }

/* Nome do FIFO */
#define n_fifosrv "SRVFIFO"  // Nome do FIFO do servidor

/* Tamanho máximo para o nome do arquivo */
#define MAX_FILE 50

/* Tamanho do buffer */
#define MAX_BUFFER 100

/* Estrutura de dados para o pedido ao servidor */
struct t_request {
    pid_t pid;                  // PID do cliente
    char n_file[MAX_FILE];      // Nome do arquivo solicitado
};

#endif // SRVFILE_H

