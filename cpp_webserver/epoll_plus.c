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

typedef struct sockinfo
{
	int fd;
	struct sockaddr_in sock;
}sockInfo;


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("eg: ./*.o port\n");
		exit(1);
	}
	int lfd, cfd;
	struct sockaddr_in serv_addr, clien_addr;
	socklen_t serv_len, clien_len;
	int port = atoi(argv[1]);

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_len = sizeof(serv_addr);

	bind(lfd, (struct sockaddr*) & serv_addr, serv_len);
	listen(lfd, 36);
	printf("Start accept...\n");
	
	int epfd = epoll_create(3000);
	if (epfd == -1)
	{
		perror("epoll_create error");
		exit(1);
	}
	sockInfo* sinfo = (sockInfo*)malloc(sizeof(sockInfo));
	sinfo->fd = lfd;
	sinfo->sock = serv_addr;
	struct epoll_event ev;
	ev.data.ptr = sinfo;
	ev.events = EPOLLIN;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl error");
		exit(1);
	}
	struct epoll_event res[2000];
	while (1)
	{
		ret = epoll_wait(epfd, res, sizeof(res) / sizeof(res[0]), -1);
		if (ret == -1)
		{
			perror("epoll_wait error");
			exit(1);
		}
		for (int i = 0; i < ret; i++)
		{
			int fd = ((sockInfo*)res[i].data.ptr)->fd;
			if (res[i].events != EPOLLIN)
			{
				continue;
			}
			if (fd == lfd)
			{
				char ipbuf[64];
				sockInfo* info = (sockInfo*)malloc(sizeof(sockInfo));
				clien_len = sizeof(clien_addr);
				cfd = accept(lfd, (struct sockaddr*) & clien_addr, &clien_len);

				info->fd = cfd;
				info->sock = clien_addr;
				struct epoll_event ev;
				ev.events = EPOLLIN;
				ev.data.ptr = (void*)info;
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
				printf("client accepted, fd = %d, IP = %s, Port = %d\n", cfd, 
					inet_ntop(AF_INET, &clien_addr.sin_addr.s_addr, ipbuf, sizeof(ipbuf)), ntohs(clien_addr.sin_port));
			}
			else
			{
				char ipbuf[1024] = { 0 };
				char buf[1024] = { 0 };
				int len = recv(fd, buf, sizeof(buf), 0);
				sockInfo* p = (sockInfo*)res[i].data.ptr;
				if (len == -1)
				{
					perror("recv error");
				}
				else if (len == 0)
				{
					inet_ntop(AF_INET, &p->sock.sin_addr.s_addr, ipbuf, sizeof(ipbuf));
					printf("client %d is disconnected, Ip = %s, Port = %d\n",
						fd, ipbuf, ntohs(p->sock.sin_port));
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
					free(p);
				}
				else
				{
					inet_ntop(AF_INET, &p->sock.sin_addr.s_addr, ipbuf, sizeof(ipbuf));
					printf("Recv data from client %d, Ip = %s, Port = %d\n",
						fd, ipbuf, ntohs(p->sock.sin_port));
					printf(" == buf == %s\n", buf);
					send(fd, buf, sizeof(buf) + 1, 0);
				}
			}
		}
	}
	close(lfd);
	free(sinfo);
	return 0;

}	