#include "srvfile.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t ativo = 1;
pid_t child_pids[100]; // Array to store the PIDs of child processes
int child_count = 0;

void sigint_handler(int sig)
{
    ativo = 0;
    printf("\nSIGINT received, shutting down the server...\n");

    // Send SIGTERM to all child processes
    for (int i = 0; i < child_count; i++)
    {
        printf("Sending SIGTERM to child process %d\n", child_pids[i]);
        kill(child_pids[i], SIGTERM);
    }

    // Remove the server FIFO
    unlink(n_fifosrv);
    printf("Server FIFO removed.\n");
}

int main(){

    int fifo_srv, fifo_cli, file;
    struct t_request request;
    char buffer[MAX_BUFFER];
    ssize_t bytes_read, bytes_written;
    char fifo_cli_name[50];

    signal(SIGINT, sigint_handler); // Set up the SIGINT handler

    unlink(n_fifosrv); // Remove the FIFO if it already exists
    if (mkfifo(n_fifosrv, 0666) < 0)
    {
        perror("Error creating server FIFO");
        exit(1);
    }

    printf("Server started. Waiting for requests...\n");

    while (ativo)
    {
        fifo_srv = open(n_fifosrv, O_RDONLY);
        if (fifo_srv < 0)
        {
            if (!ativo)
            {
                break; // If ativo is 0, it means the server should stop
            }
            perror("Error opening server FIFO");
            usleep(100000); // Wait a bit before trying again
            continue;       // If unable to open the FIFO, try again
        }

        bytes_read = read(fifo_srv, &request, sizeof(request));
        if (bytes_read <= 0)
        {
            if (!ativo)
            {
                break; // Check if SIGINT was received
            }
            close(fifo_srv); // Close the FIFO to reopen in the next loop
            usleep(100000);  // Wait a bit before trying again
            continue;        // If unable to read anything, try again
        }

        // Process the request normally (create child process)
        printf("Request received for file: %s\n", request.n_file);
        pid_t pid = fork();
        if (pid == 0)
        { // Child process
            file = open(request.n_file, O_RDONLY);
            if (file < 0)
            {
                perror("Error opening requested file");
                exit(1);
            }

            snprintf(fifo_cli_name, sizeof(fifo_cli_name), "%d", request.pid);
            fifo_cli = open(fifo_cli_name, O_WRONLY);
            if (fifo_cli < 0)
            {
                perror("Error opening client FIFO");
                close(file);
                exit(1);
            }

            while ((bytes_read = read(file, buffer, MAX_BUFFER)) > 0)
            {
                bytes_written = write(fifo_cli, buffer, bytes_read);
                if (bytes_written < 0)
                {
                    perror("Error writing to client FIFO");
                    break;
                }
            }

            close(file);
            close(fifo_cli);
            exit(0);
        }
        else if (pid > 0)
        {
            child_pids[child_count++] = pid; // Store the PID of the child process
        }
        else
        {
            perror("Error creating child process");
        }

        close(fifo_srv); // Close the FIFO after processing the request
        usleep(100000);  // Pause to avoid the loop running excessively fast
    }

    // Wait for all child processes to finish before shutting down the server
    for (int i = 0; i < child_count; i++)
    {
        waitpid(child_pids[i], NULL, 0);
    }

    unlink(n_fifosrv); // Remove the server FIFO
    printf("Server shut down. FIFO removed.\n");
    exit(0); // Ensure the main process terminates correctly
}
