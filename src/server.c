// server.c
#include "server.h"
#include "http.h"
#include "util.h"

#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>

int server_init(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        perror("socket");
        return -1;
    }

    int opt = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 8) < 0)
    {
        perror("listen");
        close(fd);
        return -1;
    }

    if (set_nonblocking(fd) < 0)
    {
        perror("set_nonblocking");
        close(fd);
        return -1;
    }

    return fd;
}

void server_run(int server_fd)
{
    fd_set master_set, readfds;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    int max_fd = server_fd;

    while (1)
    {
        readfds = master_set;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0)
        {
            perror("select");
            continue;
        }

        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (!FD_ISSET(fd, &readfds)) continue;

            if (fd == server_fd)
            {
                // New Client
                int client = accept(server_fd, NULL, NULL);

                if (client < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;

                    perror("accept");
                    continue;
                }

                if (set_nonblocking(client) < 0)
                {
                    perror("set_nonblocking");
                    close(client);
                    continue;
                }

                if (client >= FD_SETSIZE)
                {
                    fprintf(stderr, "Client socket exceeds FD_SETSIZE limit\n");
                    close(client);
                    continue;
                }

                FD_SET(client, &master_set);

                if (client > max_fd)
                    max_fd = client;    
            }
            else
            {
                // Existing client data
                if (handle_client(fd) < 0)
                {
                    close(fd);
                    FD_CLR(fd, &master_set);
                }
            }
        }
    }
}
