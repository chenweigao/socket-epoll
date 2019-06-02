#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>


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
	printf("%d", port);

	int lfd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, serv_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 地址族
	serv_addr.sin_port = htons(port); // 这两个用于转化大端或者小端

	bind(lfd, (struct sockaddr*)&serv_addr, serv_len);
	listen(lfd, 36); // 设置监听的最大个数
	printf("Start accept...\n");

	struct sockaddr_in client_addr;
	socklen_t cli_len = sizeof(client_addr);

	int epfd = epoll_create(2000);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

	struct epoll_event all[2000];
	while (1)
	{
		// epoll 实现
		// 第二个参数为传出参数
		int ret = epoll_wait(epfd, all, sizeof(all) / sizeof(all[0]), -1);
		printf("ret: %d\n", ret);
		for (int i = 0; i < ret; i++)
		{
			// 遍历所有可用的 socket 连接，all 数组中的前 ret 个元素
			int fd = all[i].data.fd;

			// 有新连接
			if (fd == lfd)
			{
				int cfd = accept(lfd, (struct sockaddr*)&client_addr, &cli_len);
				if (cfd == -1)
				{
					perror("accept error");
					exit(1);
				}
				// 将 cfd 挂到 rbtree
				struct epoll_event temp;
				temp.events = EPOLLIN;
				temp.data.fd = cfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &temp);
				
				// 打印客户端信息
				char ip[64] = { 0 };
				printf("New client IP: %s, Port: %d\n",
					inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, sizeof(ip)),
					ntohs(client_addr.sin_port)
				);
			}
			else
			{
				// 已经连接的客户端
				if (!all[i].events & EPOLLIN)
				{
					continue;
				}

				// EPOLLIN 事件
				char buf[1024] = { 0 };
				int len = recv(fd, buf, sizeof(buf), 0);

				if (len == -1)
				{
					perror("receive error");
					exit(1);
				}
				else if(len == 0)
				{
					printf("client disconnected ... \n");
					close(fd);
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				}
				else
				{
					printf(" revc buf: %s\n", buf);
					write(fd, buf, len);
				}
			}

		}
	}
	close(lfd);
	return 0;
}