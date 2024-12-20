#include "srvfile.h"
#include <signal.h>
#include <sys/wait.h>

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig) {
    keep_running = 0;
}

int main() {
    int fifo_srv, fifo_cli, file;
    struct t_request request;
    char buffer[MAX_BUFFER];
    ssize_t bytes_read, bytes_written;
    char fifo_cli_name[50];

    signal(SIGINT, sigint_handler);

    unlink(n_fifosrv);  // Remove o FIFO se já existir
    if (mkfifo(n_fifosrv, 0666) < 0) {
        perror("Erro ao criar FIFO do servidor");
        exit(1);
    }

    printf("Servidor iniciado. Aguardando pedidos...\n");

    while (keep_running) {
        fifo_srv = open(n_fifosrv, O_RDONLY);
        if (fifo_srv < 0) {
            perror("Erro ao abrir FIFO do servidor");
            break;
        }

        bytes_read = read(fifo_srv, &request, sizeof(request));
        close(fifo_srv);  // Fecha o FIFO imediatamente após a leitura

        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                // FIFO foi fechado pelo outro lado, reabra-o
                continue;
            }
            perror("Erro ao ler pedido do cliente");
            continue;
        }

        printf("Pedido recebido para o arquivo: %s\n", request.n_file);

        pid_t pid = fork();
        if (pid == 0) {  // Processo filho
            file = open(request.n_file, O_RDONLY);
            if (file < 0) {
                perror("Erro ao abrir arquivo solicitado");
                exit(1);
            }

            snprintf(fifo_cli_name, sizeof(fifo_cli_name), "%d", request.pid);
            fifo_cli = open(fifo_cli_name, O_WRONLY);
            if (fifo_cli < 0) {
                perror("Erro ao abrir FIFO do cliente");
                close(file);
                exit(1);
            }

            while ((bytes_read = read(file, buffer, MAX_BUFFER)) > 0) {
                bytes_written = write(fifo_cli, buffer, bytes_read);
                if (bytes_written < 0) {
                    perror("Erro ao escrever no FIFO do cliente");
                    break;
                }
            }

            close(file);
            close(fifo_cli);
            exit(0);
        } else if (pid < 0) {
            perror("Erro ao criar processo filho");
        } else {
            // Processo pai
            waitpid(pid, NULL, 0);  // Espera o filho terminar
        }
    }

    unlink(n_fifosrv);
    printf("Servidor encerrado. FIFO removido.\n");
    return 0;
}

