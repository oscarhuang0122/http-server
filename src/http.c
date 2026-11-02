// http.c
#include "http.h"
#include "util.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define MAX_FILE_SIZE 8192

static void send_response(int fd, int status_code, const char *status_text,
                          const char *content_type, const char *body, size_t body_len)
{
    char header[BUFFER_SIZE];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status_code, status_text, content_type, body_len);

    ssize_t n;

    n = write(fd, header, hlen);
    if (n < 0) perror("write header");

    n = write(fd, body, body_len);
    if (n < 0) perror("write body");
}

int handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];

    ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes <= 0)
    {
        if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return 0;

        return -1;
    }

    buffer[bytes] = '\0';

    printf("Request:\n%s\n", buffer);

    // Extract the requested path
    char method[8] = {0};
    char path[256] = {0};

    if (sscanf(buffer, "%7s %255s", method, path) != 2)
    {
        const char *bad_request = "<h1>400 Bad Request</h1>";

        send_response(client_fd, 400, "Bad Request", "text/html", bad_request, strlen(bad_request));

        return -1;
    }

    if (strcmp(method, "GET") != 0)
    {
        const char *method_not_allowed = "<h1>405 Method Not Allowed</h1>";

        send_response(client_fd, 405, "Method Not Allowed", "text/html", method_not_allowed, strlen(method_not_allowed));

        return -1;
    }

    if (strstr(path, "..") != NULL)
    {
        const char *forbidden = "<h1>403 Forbidden</h1>";

        send_response(client_fd, 403, "Forbidden", "text/html", forbidden, strlen(forbidden));

        return -1;
    }

    // Default to index.html if root is requested
    if (!strcmp(path, "/"))
        strcpy(path, "/index.html");

    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "www%s", path);

    // Open the file
    int fd = open(fullpath, O_RDONLY);
    if (fd < 0)
    {
        const char *not_found = "<h1>404 Not Found</h1>";
        send_response(client_fd, 404, "Not Found", "text/html", not_found, strlen(not_found));
        return 0;
    }

    static char file_buffer[MAX_FILE_SIZE];
    
    struct stat st;

    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        close(fd);
        return -1;
    }

    if (!S_ISREG(st.st_mode) || st.st_size < 0)
    {
        close(fd);
        return -1;
    }

    size_t file_size = st.st_size;

    if (file_size > MAX_FILE_SIZE)
    {
        const char *err = "File too large";
        send_response(client_fd, 500, "Internal Server Error", "text/plain", err, strlen(err));
    }
    else
    {
        size_t total_read = 0;

        while (total_read < file_size)
        {
            ssize_t n = read(fd, file_buffer + total_read, file_size - total_read);

            if (n < 0)
            {
                if (errno == EINTR)
                continue;

                perror("read file");
                close(fd);
                return -1;
            }

            if (n == 0)
            {
                fprintf(stderr, "Unexpected end of file\n");
                close(fd);
                return -1;
            }

            total_read += (size_t)n;
        }

        send_response(client_fd, 200, "OK", get_mime_type(fullpath), file_buffer, total_read);
    }

    close(fd);
    return 0;
}
