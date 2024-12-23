#include "srvfile.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t ativo = 1;
pid_t child_pids[100]; // Array para armazenar os PIDs dos processos filhos
int child_count = 0;

void sigint_handler(int sig)
{
    ativo = 0;
    printf("\nSIGINT recebido, encerrando o servidor...\n");

    // Envia SIGTERM para todos os processos filhos
    for (int i = 0; i < child_count; i++)
    {
        printf("Enviando SIGTERM para o processo filho %d\n", child_pids[i]);
        kill(child_pids[i], SIGTERM);
    }

    // Remover o FIFO do servidor
    unlink(n_fifosrv);
    printf("FIFO do servidor removido.\n");
}

int main(){
    
    int fifo_srv, fifo_cli, file;
    struct t_request request;
    char buffer[MAX_BUFFER];
    ssize_t bytes_read, bytes_written;
    char fifo_cli_name[50];

    signal(SIGINT, sigint_handler); // Configura o manipulador de SIGINT

    unlink(n_fifosrv); // Remove o FIFO se já existir
    if (mkfifo(n_fifosrv, 0666) < 0)
    {
        perror("Erro ao criar FIFO do servidor");
        exit(1);
    }

    printf("Servidor iniciado. Aguardando pedidos...\n");

    while (ativo)
    {
        fifo_srv = open(n_fifosrv, O_RDONLY);
        if (fifo_srv < 0)
        {
            if (!ativo)
            {
                break; // Se o ativo for 0, significa que o servidor deve parar
            }
            perror("Erro ao abrir FIFO do servidor");
            usleep(100000); // Espera um pouco antes de tentar novamente
            continue;       // Se não conseguir abrir o FIFO, tenta novamente
        }

        bytes_read = read(fifo_srv, &request, sizeof(request));
        if (bytes_read <= 0)
        {
            if (!ativo)
            {
                break; // Verifica se SIGINT foi recebido
            }
            close(fifo_srv); // Fecha o FIFO para reabrir no próximo loop
            usleep(100000);  // Espera um pouco antes de tentar novamente
            continue;        // Se não conseguir ler nada, tenta novamente
        }

        // Processa a requisição normalmente (criação do processo filho)
        printf("Pedido recebido para o arquivo: %s\n", request.n_file);
        pid_t pid = fork();
        if (pid == 0)
        { // Processo filho
            file = open(request.n_file, O_RDONLY);
            if (file < 0)
            {
                perror("Erro ao abrir arquivo solicitado");
                exit(1);
            }

            snprintf(fifo_cli_name, sizeof(fifo_cli_name), "%d", request.pid);
            fifo_cli = open(fifo_cli_name, O_WRONLY);
            if (fifo_cli < 0)
            {
                perror("Erro ao abrir FIFO do cliente");
                close(file);
                exit(1);
            }

            while ((bytes_read = read(file, buffer, MAX_BUFFER)) > 0)
            {
                bytes_written = write(fifo_cli, buffer, bytes_read);
                if (bytes_written < 0)
                {
                    perror("Erro ao escrever no FIFO do cliente");
                    break;
                }
            }

            close(file);
            close(fifo_cli);
            exit(0);
        }
        else if (pid > 0)
        {
            child_pids[child_count++] = pid; // Armazena o PID do processo filho
        }
        else
        {
            perror("Erro ao criar processo filho");
        }

        close(fifo_srv); // Fecha o FIFO após processar o pedido
        usleep(100000);  // Dá uma pausa para evitar o loop rodando excessivamente rápido
    }

    // Espera todos os processos filhos terminarem antes de encerrar o servidor
    for (int i = 0; i < child_count; i++)
    {
        waitpid(child_pids[i], NULL, 0);
    }

    unlink(n_fifosrv); // Remove o FIFO do servidor
    printf("Servidor encerrado. FIFO removido.\n");
    exit(0); // Assegura que o processo principal termina corretamente
}
