// http_server_prototype.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 4096

#define STATIC_MODE 1
#define MAX_CLIENTS 16
#define MAX_FILE_SIZE 8192

const char *get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext) return "text/plain";

    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".txt") ==0) return "text/plain";

    return "application/octet-stream";
}

void send_response(int client_socket, int status_code, const char *status_text,
                   const char *content_type, const char *body, size_t body_len)
{
    char header[BUFFER_SIZE];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len);

    write(client_socket, header, header_len);
    write(client_socket, body, body_len);
}

int handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int bytes = read(client_socket, buffer, sizeof(buffer) - 1);

    if (bytes <= 0)
    {
        return -1; // Client closed
    }

    printf("Request:\n%s\n", buffer);

    // Extract the requested path
    char method[8], path[1022];
    sscanf(buffer, "%7s %1021s", method, path);

    // Default to index.html if root is requested
    if (strcmp(path, "/") == 0)
    {
        strcpy(path, "/index.html");
    }

    // Remove leading slash
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "./%s", path[0] == '/' ? path + 1 : path);
    printf("Trying to open file: %s\n", file_path);

    // Open the file
    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        printf("Failed to open: %s\n", file_path);
        const char *not_found = "<h1>404 Not Found</h1>";
        send_response(client_socket, 404, "Not Found", "text/html", not_found, strlen(not_found));
    }
    else
    {
        struct stat st;
        fstat(fd, &st);
        size_t file_size = st.st_size;

        static char file_buffer[MAX_FILE_SIZE];

        if (file_size > MAX_FILE_SIZE)
        {
            const char *err = "File too large";
            send_response(client_socket, 500, "Internal Server Error",
                          "text/plain", err, strlen(err));
        }
        else
        {
            read(fd, file_buffer, file_size);
            const char *mime_type = get_mime_type(file_path);
            send_response(client_socket, 200, "OK",
                            mime_type, file_buffer, file_size);
        }

        close(fd);
    }

    return 0;
}

void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server_fd);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    listen(server_fd, 3);
    printf("Server files on http://localhost:%d\n", PORT);

    fd_set master_set, read_set;
    int max_fd;

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    max_fd = server_fd;

    while (1)
    {
        read_set = master_set;

        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0)
        {
            perror("select");
            break;
        }

        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (!FD_ISSET(fd, &read_set))
                continue;

            if (fd == server_fd)
            {
                // New Client
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd >= 0)
                {
                    set_nonblocking(client_fd);
                    FD_SET(client_fd, &master_set);
                    if (client_fd > max_fd)
                        max_fd = client_fd;
                }
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

    return 0;
}
