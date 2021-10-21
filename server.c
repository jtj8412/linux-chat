#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_CLIENT 10
#define CHATDATA 1024
#define NAMEDATA 20
#define INVALID_SOCK -1

struct user {
	int socket;
	char name[NAMEDATA];
};
typedef struct user user;

void* do_chat(void *arg);
int push_client(int c_socket);
int pop_client(int c_socket);
void filtering(char *msg);
void replaceToStar(char *msg, int len);
void send_all(int user_idx, char *msg);
int send_whisper(int user_idx, char *msg);
int create_msg(char *ret, int user_idx, char *msg);

char ESCAPE[] = "exit\n";
char ENTRY[] = "서버에 접속하였습니다!\n";
char CODE200[] = "No More Connection\n";
char WHISPER[] = "/w";
char WHISPER_ID_ERROR[] = "귓속말을 보낼 아이디를 입력해야합니다\n";
char* FILTERING[] = {"babo", "mungcheong", "ddong", "dogboy", "stupid"};

user user_list[MAX_CLIENT];
pthread_t thread;
pthread_mutex_t mutex;

main(int argc, char *argv[])
{
	int c_socket, s_socket, len, nfds = 0, i, j, n, res;
	struct sockaddr_in s_addr, c_addr;

	if (argc < 2)
	{
		printf("usage: %s port_number\n", argv[0]);
		return -1;
	}

	if (pthread_mutex_init(&mutex, NULL) != 0)
	{
		printf("Con not create mutex\n");
		return -1;
	}

	s_socket = socket(PF_INET, SOCK_STREAM, 0);

	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(atoi(argv[1]));

	if (bind(s_socket, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1)
	{
		printf("Can not Bind\n");
		return -1;
	}

	if (listen(s_socket, MAX_CLIENT) == -1)
	{
		printf("Listen Fail\n");
		return -1;
	}

	for (i = 0; i < MAX_CLIENT; i++)	user_list[i].socket = INVALID_SOCK;

	while (1)
	{
		len = sizeof(c_addr);
		c_socket = accept(s_socket, (struct sockaddr*)&c_addr, &len);

		res = push_client(c_socket);
		if (res < 0)
		{
			write(c_socket, CODE200, strlen(CODE200));
			close(c_socket);
		}
		else
		{
			write(c_socket, ENTRY, strlen(ENTRY));
			pthread_create(&thread, NULL, do_chat, (void*)res);
		}
	}
}

void* do_chat(void *arg)
{
	int user_idx = (int)arg;
	char msg[CHATDATA];
	int i;

	while (1)
	{
		memset(msg, 0, sizeof(msg));
		if ((read(user_list[user_idx].socket, msg, CHATDATA)) > 0)
		{	
			if (!strcmp(msg, ESCAPE)) { pop_client(user_list[user_idx].socket); break; }
			filtering(msg);
			if (send_whisper(user_idx, msg)) continue;
			send_all(user_idx, msg);
		}
	}
}

void filtering(char *msg)
{
	int len = sizeof(FILTERING) / sizeof(char*), i;
	char *ptr;

	for (i = 0; i < len; i++)
	{
		if ((ptr = strstr(msg, FILTERING[i])) != NULL)
			replaceToStar(ptr, strlen(FILTERING[i]));
	}
}

void replaceToStar(char *msg, int len)
{
	int i;
	for (i = 0; i < len; ++i)
	{
		*(msg + i) = '*';
	}
}

void send_all(int user_idx, char *msg)
{
	int i, len;
	char sndMsg[CHATDATA];

	len = create_msg(sndMsg, user_idx, msg);

	for (i = 0; i < MAX_CLIENT; i++)
	{
		if (user_list[i].socket == INVALID_SOCK) continue;
		if (user_list[i].socket == user_list[user_idx].socket) continue;
		write(user_list[i].socket, sndMsg, len);
	}
}

int send_whisper(int user_idx, char *msg)
{
	int idx = 0, len, i;
	char snd_msg[CHATDATA] = {}, *name;

	if (strncmp(msg, WHISPER, strlen(WHISPER))) return 0;
	idx += strlen(WHISPER);
	if (*(msg + idx)  == '\0')
	{
		write(user_list[user_idx].socket, WHISPER_ID_ERROR, strlen(WHISPER_ID_ERROR));
		return 1;
	}
	name = msg + idx + 1;
	for (idx++; *(msg + idx) != ' ' && *(msg + idx) != '\0'; idx++);	

	if (*(msg + idx) == '\0') return 1;

	*(msg + idx) = '\0';
		
	len = create_msg(snd_msg, user_idx, msg + idx + 1);

	for (i = 0; i < MAX_CLIENT; i++)
	{
		if (!strcmp(user_list[i].name, name))
		{
			write(user_list[i].socket, snd_msg, len);
			break;
		}
	}

	return 1;
}

int create_msg(char* ret, int user_idx, char *msg)
{
	return sprintf(ret, "[%s] %s", user_list[user_idx].name, msg);
}

int push_client(int c_socket)
{
	int i;
	
	for (i = 0; i < MAX_CLIENT; i++)
	{
		pthread_mutex_lock(&mutex);
		if (user_list[i].socket == INVALID_SOCK)
		{
			user_list[i].socket = c_socket;
			read(c_socket, user_list[i].name, NAMEDATA);
			printf("[%s]님이 접속하였습니다.\n", user_list[i].name);
			send_all(i, "님이 접속하였습니다\n");
			pthread_mutex_unlock(&mutex);
			return i;
		}
		pthread_mutex_unlock(&mutex);
	}

	if (i == MAX_CLIENT) return -1;
}

int pop_client(int c_socket)
{
	int i;

	close(c_socket);

	for (i = 0; i < MAX_CLIENT; i++)
	{
		pthread_mutex_lock(&mutex);
		if (user_list[i].socket == c_socket)
		{
			printf("[%s]님이 종료하였습니다\n", user_list[i].name);
			send_all(i, "님이 종료하였습니다\n");
			user_list[i].socket = INVALID_SOCK;
			pthread_mutex_unlock(&mutex);
			break;	
		}
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}
















