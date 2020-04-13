// chat_server_thread.c
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>
#include        <netinet/in.h>
#include        <sys/socket.h>
#include	<pthread.h>

void* 		do_chat(void *);

pthread_t	thread;
pthread_mutex_t	mutex;



#define         MAX_CLIENT      10
#define         CHATDATA        1024

#define         INVALID_SOCK        -1
#define			MAX_NAME		20

typedef struct {
	int clientsocket;
	char name[MAX_NAME];
} user;

user     	list_c[MAX_CLIENT];

char    	escape[] = "exit";
char    	greeting[] = "Welcome to chatting room\n";
char    	CODE200[] = "Sorry No More Connection\n";

main(int argc, char *argv[])
{
        int     c_socket, s_socket;
        struct  sockaddr_in s_addr, c_addr;
        int     len;
        int     nfds = 0;
        int     i, j, n;
        fd_set  read_fds;
	int	res;

        if (argc < 2) {
                printf("usage: %s port_number\n", argv[0]);
                exit(-1);
        }

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		printf("Can not create mutex\n");
		return -1;
	}

        s_socket = socket(PF_INET, SOCK_STREAM, 0);

        memset(&s_addr, 0, sizeof(s_addr));
        s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(atoi(argv[1]));

        if (bind(s_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1) {
                printf("Can not Bind\n");
                return -1;
        }

        if(listen(s_socket, MAX_CLIENT) == -1) {
                printf("listen Fail\n");
                return -1;
        }

	for (i = 0; i < MAX_CLIENT; i++)
		list_c[i].clientsocket = INVALID_SOCK;

	while(1) {
		len = sizeof(c_addr);
		c_socket = accept(s_socket, (struct sockaddr *)&c_addr, &len);

		res = pushClient(c_socket);
		if (res < 0) {
			write(c_socket, CODE200, strlen(CODE200));
			close(c_socket);
		} else {
			write(c_socket, greeting, strlen(greeting));
			pthread_create(&thread, NULL, do_chat, (void *) c_socket);
			printf("socket = %d\n",c_socket);
			printf("nickname = %s\n",list_c[res].name);
		}	
	}
}

void *
do_chat(void *arg) 
{
	int c_socket = (int) arg;
        char    chatData[CHATDATA];
		char commandDelim[] = "/";
		char messageDelim[] = "]";
		char userName[MAX_NAME];
		int	i, n;
		pthread_mutex_lock(&mutex);
		for(i=0;i<MAX_CLIENT;i++){
			if(c_socket == list_c[i].clientsocket)
				strcpy(userName,list_c[i].name);
		}
		pthread_mutex_unlock(&mutex);

	while(1) {
		memset(chatData, 0, sizeof(chatData));
        if ((n = read(c_socket, chatData, sizeof(chatData))) > 0 ) {
			char *name = strtok(chatData,messageDelim);		//chatData에서 이름과 메시지를 분리
			//name+=1;
			char *message = strtok(NULL,messageDelim);
			message+=1;
			if(strstr(message,commandDelim)!=NULL){	//사용자가 명령어를 입력하면 면들어오는 분기
				char *command;
				char newData[CHATDATA];
				int dest_socket;
				command = strtok(message,commandDelim);
				message = strtok(NULL,commandDelim);
				memset(newData,0,sizeof(newData));
				if((strcmp(command,"who"))==0){	//전체 사용자 출력
					char buf[CHATDATA];
					pthread_mutex_lock(&mutex);
					for(i=0;i<MAX_CLIENT;i++){
						if(list_c[i].clientsocket>0){
							sprintf(buf,"name = %s,  socket = %d\n",list_c[i].name,list_c[i].clientsocket);
							strcat(newData,buf);
						}
					}
					pthread_mutex_unlock(&mutex);
					write(c_socket,newData,strlen(newData));
				}
				else if((strcmp(command,"whoami")==0)){	//사용자 이름 확인
					char  buf[CHATDATA];
					n = sprintf(buf,"you are %s, socket = %d\n",userName,c_socket);
					write(c_socket,buf,n);
				}
				else if((strcmp(command,"changename")==0)){	//이름 바꾸기
					message+=1;
					message[strlen(message)-1]='\0';				
					strcpy(userName,message);
					pthread_mutex_lock(&mutex);
					for(i=0;i<MAX_CLIENT;i++){
						if((strcmp(name,list_c[i].name)==0)){
							strcpy(list_c[i].name ,message);
							strcpy(userName,list_c[i].name);
							printf("name = %s\n",list_c[i].name);
						}						
					}
					pthread_mutex_unlock(&mutex);
					sprintf(newData,"name -> %s\n",message);
					write(c_socket,newData,strlen(newData));
				}
				else if((dest_socket = atoi(command))>0){	//사용자가 소켓번호 직접 입력한 후 귓속말
					pthread_mutex_lock(&mutex);
					for(i=0;i<MAX_CLIENT;i++){
						if(dest_socket == list_c[i].clientsocket){
							sprintf(newData,"[%s] 's whispher : %s",userName,message);
							write(dest_socket,newData,strlen(newData));
							break;
						}
					}
					pthread_mutex_unlock(&mutex);
				}
				else{
					pthread_mutex_lock(&mutex);
					for(i=0; i < MAX_CLIENT; i++){		//사용자가 이름으로 귓속말
						if((strcmp(command,list_c[i].name)==0)){
							dest_socket = list_c[i].clientsocket;
							sprintf(newData,"[%s] 's whispher : %s",userName,message);
							write(dest_socket,newData,strlen(newData));
							break;
						}
					}
					pthread_mutex_unlock(&mutex);
				}
				
			
			}
			else{
				char buf[CHATDATA];
				n = sprintf(buf,"[%s] %s",userName,message);
               	for (i = 0; i < MAX_CLIENT; i++) {
                       	if (list_c[i].clientsocket != INVALID_SOCK) {
                               	write(list_c[i].clientsocket, buf, n);
						}
				}

               		if(strstr(chatData, escape) != NULL) {
                   		popClient(c_socket);
						break;
                    }
			}
        }
    }
}

int
pushClient(int c_socket) {
	int	i;

	for (i = 0; i < MAX_CLIENT; i++) {
		pthread_mutex_lock(&mutex);
		if(list_c[i].clientsocket == INVALID_SOCK) {
			list_c[i].clientsocket = c_socket;
			read(list_c[i].clientsocket,list_c[i].name,sizeof(list_c[i].name));	//첫 접속시 클라이언트 닉네임 저장
			pthread_mutex_unlock(&mutex);
			return i;
		}	
		pthread_mutex_unlock(&mutex);
	}

	if (i == MAX_CLIENT)
		return -1;
}

int
popClient(int s)
{
        int     i;

        close(s);

        for (i = 0; i < MAX_CLIENT; i++) {
		pthread_mutex_lock(&mutex);
                if ( s == list_c[i].clientsocket ) {
			list_c[i].clientsocket = INVALID_SOCK;
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);
        }

        return 0;
}
