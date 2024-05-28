#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define BUF_SIZE 100
#define EPOLL_SIZE 50
#define MAX_CLIENTS 10

void *client_handler(void *arg);
void error_handling(char *msg);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    pthread_t t_id;
    int epfd, event_cnt, i;

    struct epoll_event *ep_events;
    struct epoll_event event;

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    epfd = epoll_create(EPOLL_SIZE);
    if (epfd == -1)
        error_handling("epoll_create() error");

    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
    if (ep_events == NULL)
        error_handling("malloc() error");

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event) == -1)
        error_handling("epoll_ctl() error");

    while (1) {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if (event_cnt == -1) {
            puts("epoll_wait() error");
            break;
        }

        for (i = 0; i < event_cnt; i++) {
            if (ep_events[i].data.fd == serv_sock) {
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                if (clnt_sock == -1) {
                    puts("accept() error");
                    continue;
                }

                printf("Connected client: %d\n", clnt_sock);

                // 클라이언트를 처리하는 서브 쓰레드 생성
                pthread_create(&t_id, NULL, client_handler, (void *)&clnt_sock);
                pthread_detach(t_id); // 쓰레드를 디타치하여 자원 해제
            }
        }
    }

    close(serv_sock);
    close(epfd);
    free(ep_events);
    return 0;
}

void *client_handler(void *arg)
{
    int clnt_sock = *((int *)arg);
    char buf[BUF_SIZE];
    int str_len;

    while ((str_len = read(clnt_sock, buf, sizeof(buf))) != 0) {
        write(clnt_sock, buf, str_len);
    }

    close(clnt_sock);
    printf("Closed client: %d\n", clnt_sock);
    return NULL;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}