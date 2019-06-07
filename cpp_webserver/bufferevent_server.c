#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h> // 链接监听器
#include <signal.h>

// read_cb
void read_cb(struct bufferevent* bev, void* arg)
{
	// read data from bufferevent's buffer
	char buf[1024] = { 0 };
	bufferevent_read(bev, buf, sizeof(buf));
	printf("Recv: %s\n", buf);

	char* pt = "Recv your data ..";
	// send data, write to bufferevent's buffer
	bufferevent_write(bev, pt, strlen(pt) + 1);
	printf("send data to client!\n");
}

// write_cb
void write_cb(struct bufferevent* bev, void* arg)
{
	printf("write_cb is called! data is sent!\n");
}

// event_cb
void event_cb(struct bufferevent* bev, short event, void* arg)
{
	if (event & BEV_EVENT_EOF)
	{
		printf("connection closed\n");
	}
	else if (event & BEV_EVENT_ERROR)
	{
		printf("some other error\n");
	}
	// free bufferevent
	bufferevent_free(bev);
}

void signal_cb(evutil_socket_t sig, short events, void* user_data)
{
	struct event_base* base = user_data;
	struct timeval delay = { 2, 0 };

	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	event_base_loopexit(base, &delay);
}

// 连接完成之后，对应的通信操作
void listen_cb(struct evconnlistener* listener, 
	evutil_socket_t fd, struct sockaddr *addr, int len, void* ptr)
{
	// get event_base
	struct event_base* base = ptr;

	// recv data - send data
	// fd -> bufferevent
	struct bufferevent* bev;
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	// set call back to bufferevent
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

	// deafult ev_read is disable, make it enable
	// make both of them enable to void the segment fault
	bufferevent_enable(bev, EV_WRITE);
	bufferevent_enable(bev, EV_READ);

}


int main(int argc, const char* argv[])
{
	// 创建一个事件处理框架
	struct event_base* base = event_base_new();
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}


	// init serv
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(8899);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);

	// create socket, bind, listen, wait and accept
	struct evconnlistener* listen = NULL;

	// 有新连接的时候，listen_cb 被调用
	listen = evconnlistener_new_bind(base, listen_cb, (void*)base,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, 
		(struct sockaddr*)&serv, sizeof(serv));
	if (!listen) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	struct event* signal_event;


	// SIGINT in <signal.h> 程序终止信号：ctrl + c
	signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base);

	if (!signal_event || event_add(signal_event, NULL) < 0)
	{
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	// event loop
	event_base_dispatch(base);

	// free
	evconnlistener_free(listen);
	event_free(signal_event);
	event_base_free(base);

	return 0;
}