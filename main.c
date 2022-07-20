#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define HOST      "irc.chat.twitch.tv"
#define PORT      "6667"

static int sfd;

char
*pformat(char *m) {
	char *fmt = calloc(sizeof(char), strlen(m)+2);
	strcpy(fmt, m);
	strcat(fmt, "\r\n");
	return fmt;
}	

int
sendp(char *m)
{
	char *temp = pformat(m);
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
	while (!bs)
		bs = recv(sfd, msg, 1024,0);
	fprintf(stdout, "message in:\n%sbytes sent:\t%d\n\n",msg, bs);
	return bs;
}

int
main(void)
{
	int s;
	struct addrinfo *i, *p, hints = { 0 };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
    // Get Twitch IRC's ip address
	if((s = getaddrinfo(HOST, PORT, &hints, &i)) != 0) {
		fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(s));
		return 2;
	}

	
	if((sfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1)
	{
		fprintf(stderr, "Error creating socket\n");
		freeaddrinfo(i);
		return 2;
	}


	connect(sfd, i->ai_addr, i->ai_addrlen);
	
	char *buf = calloc(sizeof(char), 2048);
	sendp("CAP REQ :twitch.tv/tags twitch.tv/commands");
	recvp(buf);
	
	// logging in as anonymous
	sendp("PASS SCHMOOPIIE");
    sendp("NICK justinfan1337");
	recvp(buf);
	
	free(buf);
	close(sfd);
	freeaddrinfo(i);
	return 0;
}
