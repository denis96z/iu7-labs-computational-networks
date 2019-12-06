#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define BUFSIZE 512

int main(int argc, char **argv) {
    char buf[BUFSIZE];

    if (argc != 2) {
        printf("Usage: echoc <ip>\nEx.:   echoc 10.30.0.2\n");
        exit(0);
    }

    int sk = 0;
    if ((sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(1996);

    if (connect(sk, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }
 
    while (1) {
        printf("> ");
        fgets(buf, BUFSIZE, stdin);
        size_t len = strlen(buf);
        if (len == 0)
            continue;
        buf[len-1] = '\0';
        if (strcmp(buf, "/q") == 0)
            break;

        if (send(sk, buf, len, 0) < 0) {
            perror("send");
            exit(1);
        }

        len = recv(sk, buf, BUFSIZE, 0);
        if (len < 0) {
            perror("recv");
            exit(1);
        } else if (len == 0) {
            printf("Remote host has closed the connection\n");
            exit(1);
        }
        
        buf[len] = '\0';
        printf("<< %s\n", buf);
    }

    sprintf(buf, "/q");
    if (send(sk, buf, strlen(buf), 0) < 0) {
        perror("send");
        exit(1);
    }

    close(sk);
    return 0;   
}
