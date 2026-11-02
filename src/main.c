// main.c
#include "server.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int server_fd = server_init(8080);

    if (server_fd < 0)
    {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    printf("Server running at http://localhost:8080\n");

    server_run(server_fd);

    close(server_fd);
    return 0;
}