#include<stdio.h>
#include"commondef.h"
#include"receive.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include"Show.h"
#include"message.h"
#include"screen.h"
#include<stdlib.h>
#include<pthread.h>
#include"logic.h"
#include"stcp_client.h"

extern pthread_cond_t cond;
extern pthread_mutex_t mutex;

void *receive(void *threadid)
{
	while (1)
	{
		if (stcp_client_recv(sockfd, recvBuffer, sizeof(Package)) != -1)
		{
			handle1(recvBuffer);
			menset(recvBuffer,'\0',BUFFER);
		}
		else {printf("recv error!\n"); exit(-1);}
	}
}

void handle1(char buffer[])
{
	setbuf(stdout,NULL);
	Package* package = (Package* )buffer;
	int type = (int)(package->type) - 48;

	printf("%d\n",type);	

	if (type == 2)
	{
		//int i = 0;
		Acklogin* ack;
		ack = (Acklogin *)(package->content);
		//print("daxing youxi duizhan pingtai",3);
		if (ack->ack == '1')
		{
			//printf("change interface!\n");
			main_show(0);
			interface = 1;
			game_status = 0;
			//printf("WHY!!! 6");
		} 
		else
		{
			printf("user name exist\nplease input another user name\n");
			char username[16];
			scanf("%s",username);
			menset(sendBuffer,'\0',BUFFER);
			Package* user = (Package* )sendBuffer;;
			user->type = '1';
			Login* log = (Login* )user->content;
			strcpy(log->username, username);
			stcp_client_send(sockfd,sendBuffer,BUFFER);
			strcpy(user_name,username);
		}
		//break;
	}else;
	if (type == 3)
	{
		Userlist* list = (Userlist* )package->content;
		int num = (int )list->N - 48;
		user_change(list->content,list->status,num);
		//break;
	}else;
	if (type == 5)
	{
		Gameconnect* connect = (Gameconnect* )package->content;
		int ctype = (int)connect->type -48;
		printf("%d",ctype);
		if( ctype==2)
			//if (interface != 4 && interface != 5)
			{
				interface = 2;
				game_request(connect->username1,0);
				//printf("wocao nima! 3");
			}		
			else;
			//break;
		if (ctype==4)
		{
			
			if (connect->status == '2')
			{
				interface = 1;
				main_show(0);
			}else;
			if (connect->status == '1')
			{
				printf("before");
				if (connect->game == '1')
				{
					printf("after");
					interface = 4;
					game_status = 1;
					one_playing(0,3,3);
				}else;
				if (connect->game == '2')
				{
					game_status = 2;
					if (connect->first == '1')
					{
						interface = 8;
						two_playing(0);
					}
					else
					{		
						interface = 9;
						two_result(-1);
					}
				}else;
			}else;
			//printf("diu ni lou mu! 4");
			//break;
		}else;
		//break;
	}else;
	if (type == 4)
	{
		Gameaction* action = (Gameaction* )package->content;
		if (game_status == 1)
		{
			interface = 5;
			if (strcmp(action->username1,user_name) == 0)
			{
				if (action->status1[0] == '0')
				{
					one_result(0,0,action->status2[0]);
					game_status = 0;
				}
				else
					if (action->status2[0] == '0')
					{
						one_result(1,action->status1[0],0);
						game_status = 0;
					}
					else
						one_result(-1,action->status1[0],action->status2[0]);
			}
			else
			{
				if (action->status2[0] == '0')
				{
					one_result(0,0,action->status1[0]);
					game_status = 0 ;
				}
				else
					if (action->status1[0] == '0')
					{
						one_result(1,action->status2[0],0);
						game_status = 0;
					}
					else
						one_result(-1,action->status2[0],action->status1[0]);
			}
		}else;
		if (game_status == 2)
		{
			if (action->action1 == '1')
			{
				interface = 8;
				two_playing(0);
			}else;
			if (action->action1 == '0')
			{
				interface = 9;
				if (strcmp(action->username1,user_name) == 0)
				{
					if (action->status1[0] == '0')
					{
						two_result(0);
						game_status = 0;
						interface = 10;
					}
					else
						if (action->status2[0] == '0')
						{
							two_result(1);
							game_status = 0;
							interface = 10;
						}
						else
							two_result(-1);
				}	
				else
				{
					if (action->status2[0] == '0')
					{
						two_result(0);
						game_status = 0 ;
						interface = 10;
					}
					else
						if (action->status1[0] == '0')
						{
							two_result(1);
							game_status = 0;
							interface = 10;
						}
						else
							two_result(-1);
				}
			}else;	
		}
		//break;
	}else;
	if (type == 6)
	{
		Talk* talk = (Talk* )package->content;
		char temp[100];
		sprintf(temp,"%s : %s",talk->username,talk->content);
		print(temp,20);
		output();
	}else;
	//pthead_cond_signal();
	//if (type!=3) pthread_mutex_unlock(&mutex); else;
			
}




