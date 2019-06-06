#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <event2/event.h>


// call back function
void write_cb(evutil_socket_t fd, short what, void* arg)
{
	char buf[1024] = { 0 };
	static int num = 0;
	sprintf(buf, "hello, world == %d\n", num++);
	write(fd, buf, strlen(buf) + 1);
}


int main(int argc, char* argv[])
{
	// delete if exits
	unlink("myfifo");
	// create FIFO
	mkfifo("myfifo", 0664);

	int fd = open("myfifo", O_WRONLY | O_NONBLOCK);
	if (fd == -1)
	{
		perror("open failed");
		exit(1);
	}

	// write fifo using libevent
	struct event_base* base = NULL;
	base = event_base_new();

	// create event
	struct event* ev;
	//detects whether the write buffer is writable, 4k maxsize of buffer
	ev = event_new(base, fd, EV_PERSIST | EV_WRITE, write_cb, NULL);

	// add this event to base, tv = NULL or {0, 100}, time
	event_add(ev, NULL);

	// event loop
	event_base_dispatch(base);

	// free ev and base
	event_free(ev);
	event_base_free(base);

	close(fd);
	return 0;
}