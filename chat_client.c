// chat_client_thread.c
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>
#include        <netinet/in.h>
#include        <sys/socket.h>
#include        <sys/select.h>
#include	<pthread.h>
#include	<signal.h>

#define         CHATDATA        1024

void* 		do_send_chat(void *);
void* 		do_receive_chat(void *);

pthread_t	thread_1, thread_2;

char            escape[] = "exit";
char            nickname[20];

main(int argc, char *argv[])
{
        int     c_socket;
        struct  sockaddr_in c_addr;
        int     len;
        char    chatData[CHATDATA];
        char    buf[CHATDATA];
        int     nfds;
        fd_set  read_fds;
        int     n;

        if (argc < 3) {
                printf("usage : %s ip_address port_number\n", argv[0]);
                exit(-1);
        }

        c_socket = socket(PF_INET, SOCK_STREAM, 0);

        memset( &c_addr, 0, sizeof(c_addr) );
        c_addr.sin_addr.s_addr = inet_addr(argv[1]);
        c_addr.sin_family = AF_INET;
        c_addr.sin_port = htons(atoi(argv[2]));

        printf("Input Nickname : ");
        scanf("%s", nickname);

        if (connect(c_socket, (struct sockaddr *)&c_addr, sizeof(c_addr)) == -1) {
                printf("Can not connect\n");
                return -1;
        }

        write(c_socket,nickname,sizeof(nickname)); //서버에 닉네임 전송

	pthread_create(&thread_1, NULL, do_send_chat, (void *) c_socket);
	pthread_create(&thread_2, NULL, do_receive_chat, (void *) c_socket);

	pthread_join(thread_1, NULL);
	pthread_join(thread_2, NULL);

	close(c_socket);
}

void *
do_send_chat(void* arg)
{
	char	chatData[CHATDATA];
	char	buf[CHATDATA];
	int	n;
	int	c_socket = (int) arg;		// client socket

	while(1) {
		memset(buf, 0, sizeof(buf));
                if ((n = read(0, buf, sizeof(buf))) > 0 ) {
                	sprintf(chatData, "[%s] %s", nickname, buf);
                        write(c_socket, chatData, strlen(chatData));

                        if (!strncmp(buf, escape, strlen(escape))) {
				pthread_kill(thread_2, SIGINT);
                        	break;
                        }
                }
	}
}

void *
do_receive_chat(void* arg)
{
	char	chatData[CHATDATA];
	int	n;
	int	c_socket = (int) arg;		// client socket

	while(1) {
		memset(chatData, 0, sizeof(chatData));
                if ((n = read(c_socket, chatData, sizeof(chatData))) > 0 ) {
                        write(1, chatData, n);
                }
	}
}
