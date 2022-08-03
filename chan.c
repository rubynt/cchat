#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <fcntl.h>
#define MAXEVENTS  64
#define BUF_LENGHT 2048
#define HOST       "irc.chat.twitch.tv"
#define PORT       "6667"

// #define ERR(x) (x == -1)?(perror ("fcntl");return -1;)

static int sfd;

__attribute__((always_inline)) inline int
err(int x, char *err)
{
	if (x == -1)
	{
		perror(err);
		return -1;
	}
}

//Create and connect
static int 
cac(char *host, char *port)
{
	int s;
	struct addrinfo *i, *p, hints = { 0 };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
    // Get Twitch IRC's ip address
	if((s = getaddrinfo(host, port, &hints, &i)) != 0) {
		fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(s));
		return 2;
	}

	
	if((s = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1)
	{
		fprintf(stderr, "Error creating socket\n");
		freeaddrinfo(i);
		return 2;
	}
	
	connect(s, i->ai_addr, i->ai_addrlen);
	freeaddrinfo(i);
	return s;
}

static int
sfd_nonblock(int sfd)
{
	int f, s;

	f = fcntl(sfd, F_GETFL, 0);
	err(f,"fcntl");

	f |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, f);
	err(f,"fcntl");

	return 0;
}

static char
*pformat(char *m) {
	char *fmt = calloc(sizeof(char), strlen(m)+2);
	strcpy(fmt, m);
	strcat(fmt, "\r\n");
	return fmt;
}	

int
sendp(char *msg)
{
	char *temp = pformat(msg);
	int bs = send(sfd, temp, strlen(temp),0);
	fprintf(stdout, "message out:\t%sbytes recieved:\t%d\n\n", temp, bs);
	free(temp);
	return bs;
}

int
recvp(char *msg)
{
	fprintf(stdout, "waiting...\n");
	int bs = 0;
	bs = recv(sfd, msg, 1024,0);
	if (!(strcmp(msg, "PING :tmi.twitch.tv"))) {
		fprintf(stdout, "%s\n",msg);		
		sendp("PONG :tmi.twitch.tv");
		return bs;
	}
	fprintf(stdout, "message in:\n%sbytes sent:\t%d\n\n",msg, bs);
	return bs;
}

int
main(void)
{
	int s,efd;
	
	sfd = cac(HOST,PORT);
	
	sfd_nonblock(sfd);
	
	efd = epoll_create1(0);
	err(efd, "Epoll creation failed\n");
	
	struct epoll_event event =  { 0 };
	event.data.fd = sfd;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
	err(s, "epoll_ctl");

	// Requesting permissions
   	sendp("CAP REQ :twitch.tv/tags twitch.tv/commands");
	// logging in as anonymous
	sendp("PASS SCHMOOPIIE");
    sendp("NICK justinfan1337");
	
	for (;;) {
		struct epoll_event *events = calloc(MAXEVENTS, sizeof(event));
		
		int nevents = epoll_wait(efd, events, MAXEVENTS, -1);
		err(s, "epoll_wait()");
		
		for (int n = 0; n < nevents; n++) {
			
			// error case
			if (events[n].events & EPOLLERR)
				err(-1, "epoll error");

			if (events[n].events & EPOLLHUP)
				err(-1, "Unexpected hangup on socket");
			
			char *buf = calloc(BUF_LENGHT, sizeof(char));
			recvp(buf);
			free(buf);
		}
		free(events);
	}
	close(sfd);
	return 0;
}
