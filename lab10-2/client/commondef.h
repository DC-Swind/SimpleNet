#ifndef COMMONDEF_H
#define COMMONDEF_H

#define BUFFER 1024
#define PORT 6667
//#define address "114.212.133.148"
//#define address "192.168.136.129"
#define address "192.168.1.100"

//#include<pthread.h>

int sockfd;
char sendBuffer[BUFFER], recvBuffer[BUFFER];
int game_status;
int interface;
char user_name[16];
char user_list[16*128];
char scr[24*80];


struct Ack{
	int type;
	int login; //0=false 1=successed t=1
	int competitor; //0=false 1=true t=2
	int score_myself;
	int score_comp;  // score t=3
};
typedef struct Ack Ack;


#endif
