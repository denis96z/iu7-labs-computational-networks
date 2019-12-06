#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_HOST "0.0.0.0"
#define SERVER_PORT 5512

#define BUFF_SIZE 512

#define MUX_SIZE 512
static int mux_fds[MUX_SIZE] = { 0 };

#define setnoblock(fd) \
    fcntl((fd), F_SETFL, fcntl((fd), F_GETFL, 0) | O_NONBLOCK)

#define log_d(fmt, ...) \
    fprintf(stdout, "[D] "fmt" [echosrv.c:%d]\n", ##__VA_ARGS__, __LINE__)

#define log_i(fmt, ...) \
    fprintf(stdout, "[I] "fmt"\n", ##__VA_ARGS__)

#define log_e(fmt, ...) \
    fprintf(stderr, "[E] "fmt": %s [echosrv.c:%d]\n", ##__VA_ARGS__, strerror(errno), __LINE__)

#define STRLN(x) (x), (sizeof(x) - 1)

#define syscall_check(sfunc, on_err, ...) \
    do {                                  \
        if ((sfunc(__VA_ARGS__)) < 0) {   \
            log_e(#sfunc);                \
            on_err;                       \
        } else {                          \
            log_d(#sfunc"() ok");         \
        }                                 \
    } while(0);

#define syscall_check_err(sfunc, ...)   \
    do {                                \
        if ((sfunc(__VA_ARGS__)) < 0) { \
            log_e(#sfunc);              \
            exit(1);                    \
        } else {                        \
            log_d(#sfunc"() ok");       \
        }                               \
    } while(0);

int main(void) {
    int sock_fd = 0;
    syscall_check_err(sock_fd = socket, AF_INET, SOCK_STREAM, 0);
    syscall_check_err(setnoblock, sock_fd);

    log_d("sock_fd = %d", sock_fd);

    struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_addr   = inet_addr(SERVER_HOST),
            .sin_port   = htons(SERVER_PORT),
            .sin_zero   = { 0 },
    };
    syscall_check_err(bind, sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    syscall_check_err(listen, sock_fd, SOMAXCONN);

    log_i("server started on %s:%d", SERVER_HOST, SERVER_PORT);

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);

        int max_fd = sock_fd;
        for (size_t i = 0; i < MUX_SIZE; ++i) {
            int fd = mux_fds[i];
            if (fd == 0)
                continue;

            FD_SET(fd, &read_fds);
            if (fd > max_fd) {
                max_fd = fd;
            }
        }

        struct timeval timeout = {
                .tv_sec  = 5,
                .tv_usec = 0,
        };

        int num_events = 0;
        syscall_check_err(num_events = select, max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (num_events == 0) {
            log_d("no events");
            continue;
        }

        if (FD_ISSET(sock_fd, &read_fds)) {
            struct sockaddr_in remote_sock_addr;
            socklen_t addr_size = sizeof(remote_sock_addr);

            int tmp_fd = sock_fd;
            int remote_sock_fd = 0;
            syscall_check_err(remote_sock_fd = accept, sock_fd,
                    (struct sockaddr *)&remote_sock_fd, &addr_size);
            sock_fd = tmp_fd;

            log_d("new fd = %d", remote_sock_fd);
            syscall_check_err(setnoblock, remote_sock_fd);

            int is_sock_saved = 0;
            for (size_t i = 0; i < MUX_SIZE; ++i) {
                if (mux_fds[i] == 0) {
                    mux_fds[i] = remote_sock_fd;
                    is_sock_saved = 1;
                    break;
                }
            }
            if (!is_sock_saved) {
                syscall_check_err(close, remote_sock_fd);
            }
        }

        for (size_t i = 0; i < MUX_SIZE; ++i) {
            int fd = mux_fds[i];
            if (fd == 0)
                continue;

            if (FD_ISSET(fd, &read_fds)) {
                char buff[BUFF_SIZE];

                log_d("current fd = %d", fd);

                int n = 0;
                syscall_check(n = recv, goto cleanup, fd, buff, BUFF_SIZE, 0);
                if (n == 0) {
                    log_d("connection reset");
                    goto cleanup;
                }

                buff[n] = '\0';
                log_d("message: \"%s\"", buff);

                if (strncmp(buff, STRLN("/q")) == 0)
                    goto cleanup;

                for (size_t j = 0; j < MUX_SIZE; ++j) {
                    int client_fd = mux_fds[j];
                    if (client_fd == 0)
                        continue;

                    syscall_check(send, goto client_cleanup, client_fd, buff, n, 0);
                    continue;

                client_cleanup:
                    syscall_check(close, ;, client_fd);
                    mux_fds[j] = 0;
                }

                continue;

            cleanup:
                close(fd);
                mux_fds[i] = 0;
            }
        }
    }
}
