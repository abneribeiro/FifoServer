#include "srvfile.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Verifica se o número de argumentos é correto
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nome_do_arquivo>\n", argv[0]);
        exit(1);
    }

    int fifo_srv, fifo_cli;
    struct t_request request;
    char buffer[MAX_BUFFER];
    ssize_t bytes_read;
    char fifo_cli_name[50];

    // Preenche a estrutura de pedido com o PID e o nome do arquivo
    request.pid = getpid();
    strncpy(request.n_file, argv[1], MAX_FILE - 1);
    request.n_file[MAX_FILE - 1] = '\0';  // Garante terminação nula

    // Cria o nome do FIFO do cliente baseado no PID
    snprintf(fifo_cli_name, sizeof(fifo_cli_name), "%d", request.pid);
    unlink(fifo_cli_name);  // Remove o FIFO se já existir
    if (mkfifo(fifo_cli_name, 0666) < 0) {
        perror("Erro ao criar FIFO do cliente");
        exit(1);
    }

    // Abre o FIFO do servidor para escrita
    fifo_srv = open(n_fifosrv, O_WRONLY);
    if (fifo_srv < 0) {
        perror("Erro ao abrir FIFO do servidor");
        unlink(fifo_cli_name);
        exit(1);
    }

    // Envia o pedido ao servidor
    if (write(fifo_srv, &request, sizeof(request)) != sizeof(request)) {
        perror("Erro ao enviar pedido ao servidor");
        unlink(fifo_cli_name);
        exit(1);
    }

    // Abre o FIFO do cliente para leitura
    fifo_cli = open(fifo_cli_name, O_RDONLY);
    if (fifo_cli < 0) {
        perror("Erro ao abrir FIFO do cliente");
        unlink(fifo_cli_name);
        exit(1);
    }

    // Lê o conteúdo do arquivo enviado pelo servidor e escreve na saída padrão
    printf("Conteúdo do arquivo '%s':\n", argv[1]);
    while ((bytes_read = read(fifo_cli, buffer, MAX_BUFFER)) > 0) {
        if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
            perror("Erro ao escrever na saída padrão");
            break;
        }
    }

    // Verifica se houve erro na leitura do FIFO do cliente
    if (bytes_read < 0) {
        perror("Erro ao ler do FIFO do cliente");
    }

    // Fecha os FIFOs e remove o FIFO do cliente
    close(fifo_cli);
    close(fifo_srv);
    unlink(fifo_cli_name);

    return 0;
}
