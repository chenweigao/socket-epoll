#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <arpa/inet.h>


void read_cb(struct bufferevent* bev, void* arg)
{
	char buf[1024] = { 0 };

	int len = bufferevent_read(bev, buf, sizeof(buf));
	printf("Recv: %s\n", buf);

	// bufferevent_write(bev, buf, len + 1);
	printf("Success send data\n");
}

void write_cb(struct bufferevent* bev, void* arg)
{
	printf("write_cb: Sent data to server!\n");
}

void event_cb(struct bufferevent* bev, short what, void* arg)
{
	if (what & BEV_EVENT_EOF)
	{
		printf("connection closed\n");
	}
	else if(what & BEV_ERROR)
	{
		printf("some other error\n");
	}
	else if (what & BEV_EVENT_CONNECTED)
	{
		printf("connected to server!\n");
		return;
	}
	bufferevent_free(bev);
}

void read_terminal(evutil_socket_t fd, short what, void* arg)
{
	// 读取终端过来的数据
	char buf[1024] = { 0 };
	int len = read(fd, buf, sizeof(buf));
	
	// 将数据发送给 server
	struct bufferevent* bev = arg;
	bufferevent_write(bev, buf, len + 1);
	printf("Send to server!\n");

}

int main(int argc, char* argv[])
{
	// create event process frame
	struct event_base* base = event_base_new();
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	// serv.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr.s_addr);
	serv.sin_port = htons(8899);

	// create evenets
	// connect server
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	struct bufferevent* bev = NULL;
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);


	bufferevent_socket_connect(bev, (struct sockaddr*)& serv, sizeof(serv));
	
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

	bufferevent_enable(bev, EV_READ);
	bufferevent_enable(bev, EV_WRITE);

	// stdin event
	struct event* ev_in = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, read_terminal, bev);
	event_add(ev_in, NULL);

	event_base_dispatch(base);

	event_base_free(base);
}