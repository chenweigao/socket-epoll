#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		printf("eg: ./*.o port\n");
		exit(1);
	}
	struct sockaddr_in serv_addr;
	socklen_t serv_len = sizeof(serv_addr);
	int port = atoi(argv[1]);
	printf("server at port: [%d]\n", port);

	int lfd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, serv_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// serv_addr.sin_addr.s_addr = inet_addr("192.168.1.1");
	serv_addr.sin_port = htons(port);

	bind(lfd, (struct sockaddr*) & serv_addr, serv_len);
	listen(lfd, 36);
	printf("Start accept...\n");

	struct sockaddr_in client_addr;
	socklen_t cli_len = sizeof(client_addr);

	int epfd = epoll_create(2000);

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = lfd;

	epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

	struct epoll_event all[2000];

	while (1)
	{
		int ret = epoll_wait(epfd, all, sizeof(all) / sizeof(all[0]), -1);
		printf("ret: %d\n", ret);
		for (int i = 0; i < ret; i++)
		{
			int fd = all[i].data.fd;

			if (fd == lfd)
			{
				int cfd = accept(lfd, (struct sockaddr*) & client_addr, &cli_len);
				if (cfd == -1)
				{
					perror("accept error");
					exit(1);
				}

				int flag = fcntl(cfd, F_GETFL);
				flag |= O_NONBLOCK;
				fcntl(cfd, F_SETFL, flag);

				struct epoll_event temp;
				temp.events = EPOLLIN | EPOLLET;
				// temp.events = EPOLLIN | EPOLLET;

				temp.data.fd = cfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &temp);

				char ip[64] = { 0 };
				printf("New client IP: %s, Port: %d\n",
					inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, sizeof(ip)),
					ntohs(client_addr.sin_port)
				);
			}
			else
			{
				if (!all[i].events & EPOLLIN)
				{
					continue;
				}

				char buf[1024] = { 0 };
				int len;

				while ((len = recv(fd, buf, sizeof(buf), 0)) > 0)
				{
					write(STDOUT_FILENO, buf, len);

					send(fd, buf, len, 0);
				}

				if (len == 0)
				{
					printf("client disconncted!\n ");
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					if (ret == -1)
					{
						perror("epoll_ctl - error");
						exit(1);
					}
					close(fd);
				}
				else if (len == -1)
				{
					if (errno == EAGAIN)
					{
						continue;
						//printf("buffer data is empty!\n");
					}
					else
					{
						printf("recv error! len == -1 \n");
						perror("receive error");
						exit(1);
					}
				}
			}

		}
	}
	close(lfd);
	return 0;
}