#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#define CHATDATA 1024
#define NAMEDATA 20

void* do_send_chat(void *);
void* do_receive_chat(void *);

pthread_t thread_1, thread_2;

char ESCAPE[] = "exit";
char nickname[NAMEDATA];

main(int argc, char *argv[])
{
	int c_socket, len, nfds, n;
	struct sockaddr_in c_addr;
	char msg[CHATDATA], buf[CHATDATA];

	if (argc < 3)
	{
		printf("usage : %s ip_address port_number\n", argv[0]);
		return -1;
	}

	c_socket = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_addr.s_addr = inet_addr(argv[1]);
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(atoi(argv[2]));

	if (connect(c_socket, (struct sockaddr*)&c_addr, sizeof(c_addr)) == -1)
	{
		printf("Can not connect\n");
		return -1;
	}

	printf("Input Nickname : ");
	scanf("%s", nickname);
	write(c_socket, nickname, strlen(nickname));

	pthread_create(&thread_1, NULL, do_send_chat, (void*)c_socket);
	pthread_create(&thread_2, NULL, do_receive_chat, (void*)c_socket);
	pthread_join(thread_1, NULL);
	pthread_join(thread_2, NULL);

	close(c_socket);
}

void* do_send_chat(void *arg)
{
	char msg[CHATDATA];
	int n, c_socket = (int)arg;

	while(1)
	{
		memset(msg, 0, CHATDATA);
		if ((n = read(0, msg, CHATDATA)) > 0)
		{
			write(c_socket, msg, CHATDATA);
			if (!strncmp(msg, ESCAPE, strlen(ESCAPE)))
			{
				pthread_kill(thread_2, SIGINT);
				break;
			}
		}
	}
}

void* do_receive_chat(void *arg)
{
	char chatData[CHATDATA];
	int n, c_socket = (int)arg;
	
	while (1)
	{
		memset(chatData, 0, sizeof(chatData));
		if ((n = read(c_socket, chatData, sizeof(chatData))) > 0) write(1, chatData, n);
	}
}

























