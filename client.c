#include "srvfile.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Check if the number of arguments is correct
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    int fifo_srv, fifo_cli;
    struct t_request request;
    char buffer[MAX_BUFFER];
    ssize_t bytes_read;
    char fifo_cli_name[50];

    // Fill the request structure with the PID and the filename
    request.pid = getpid();
    strncpy(request.n_file, argv[1], MAX_FILE - 1);
    request.n_file[MAX_FILE - 1] = '\0';  // Ensure null termination

    // Create the client's FIFO name based on the PID
    snprintf(fifo_cli_name, sizeof(fifo_cli_name), "%d", request.pid);
    unlink(fifo_cli_name);  // Remove the FIFO if it already exists
    if (mkfifo(fifo_cli_name, 0666) < 0) {
        perror("Error creating client FIFO");
        exit(1);
    }

    // Open the server's FIFO for writing
    fifo_srv = open(n_fifosrv, O_WRONLY);
    if (fifo_srv < 0) {
        perror("Error opening server FIFO");
        unlink(fifo_cli_name);
        exit(1);
    }

    // Send the request to the server
    if (write(fifo_srv, &request, sizeof(request)) != sizeof(request)) {
        perror("Error sending request to server");
        unlink(fifo_cli_name);
        exit(1);
    }

    // Open the client's FIFO for reading
    fifo_cli = open(fifo_cli_name, O_RDONLY);
    if (fifo_cli < 0) {
        perror("Error opening client FIFO");
        unlink(fifo_cli_name);
        exit(1);
    }

    // Read the file content sent by the server and write to standard output
    printf("Content of file '%s':\n", argv[1]);
    while ((bytes_read = read(fifo_cli, buffer, MAX_BUFFER)) > 0) {
        if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
            perror("Error writing to standard output");
            break;
        }
    }

    // Check if there was an error reading from the client's FIFO
    if (bytes_read < 0) {
        perror("Error reading from client FIFO");
    }

    // Close the FIFOs and remove the client's FIFO
    close(fifo_cli);
    close(fifo_srv);
    unlink(fifo_cli_name);

    return 0;
}
