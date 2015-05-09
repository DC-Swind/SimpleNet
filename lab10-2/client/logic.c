#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include"logic.h"
#include"commondef.h"
#include"message.h"
#include"Key_Event.h"
#include"Show.h"
#include<unistd.h>
#include<pthread.h>
#include"Time.h"
#include<sys/signal.h>
#include"stcp_client.h"

pthread_cond_t cond;
pthread_mutex_t mutex;
int countdown = 0;

void menset(char* str, char zero, int size)
{
	int i=0;
	for (i=0;i<size;i++)
	{
		str[0] = zero;
		str = str + 1;
	}
}

void logic()
{
setbuf(stdout,NULL);
while (1)
{
	//pthread_mutex_lock(&mutex);
	int record = interface;
	menset(sendBuffer,'\0',BUFFER);

	rewind(stdin);
	//printf("%d %d\n",interface,record);	
	
	if (record == 0)
	{
		printf("Please input your user name!\n");
		char username[16];
		scanf("%s",username);
		menset(sendBuffer,'\0',BUFFER);
		Package* user = (Package* )sendBuffer;;
		user->type = '1';
		Login* log = (Login* )user->content;
		strcpy(log->username, username);
		stcp_client_send(sockfd,sendBuffer,BUFFER);
		strcpy(user_name,username);
		//break;
	}else;
	if (record == 1)
	{
		int loc = 0;
		while (1)
		{
			//fflush(stdin);
			while (!kbhit() && interface ==1);
			if (interface !=1) break;else; 
			char c = getchar();

			//printf("press %c",c);
			if (c == 'w')
			{ loc--; loc = loc>0?loc:0;main_show(loc);printf("WHAT! 7%d %d",interface,record);continue;}
			else;
			if (c == 's')
			{ loc++; loc= loc>4?4:loc;main_show(loc);printf("WHERE! 8%d %d",interface,record);continue;}
			else;
			if (c == 'k')
			{
				switch (loc)
				{
				case 0:
					interface = 3;
					choose_one(0);
					break;
				case 1:
					interface = 7;
					choose_two(0);
					break;
				case 2:
					interface = 11;
					state();
					break;
				case 3:
					interface = 6;
					choose_chat(0);
					break;
				case 4:
					exit(0);
				}
				break;
			}else;
		}
	}else;
	if (record == 2)
	{
		int loc = 0;
		Package* pack;
		Gameconnect* con;
		while (1)
		{
			//fflush(stdin);			
			while (!kbhit() && interface==2);
			if (interface!=2) break;else;
			char c = getchar();

			if (c == 'w')
			{ loc--; loc = loc>0?loc:0;game_request(" ",loc);printf("shit! 1");continue;}
			else;
			if (c == 's')
			{ loc++; loc= loc>1?1:loc;game_request(" ",loc);printf("fuck! 2");continue;}
			else;
			if (c == 'k')
			{
				switch (loc)
				{
				case 0:
					
					pack = (Package* )sendBuffer;
					pack->type = '5';
					con = (Gameconnect* )pack->content;
					con->type = '3';
					con->status = '1';
					con->game = '2';
					stcp_client_send(sockfd,sendBuffer,BUFFER);
					break;
				case 1:
					
					pack = (Package* )sendBuffer;
					pack->type = '5';
					con = (Gameconnect* )pack->content;
					con->type = '3';
					con->status = '2';
					con->game = '1';
					stcp_client_send(sockfd,sendBuffer,BUFFER);
					break;
				}
				break;
			}else;
		}
	}else;
	if (record == 3)
	{
		int loc = 0;
		while (1)
		{
			//fflush(stdin);
			while (!kbhit() && interface==3);
			if (interface!=3) break;else;
			char c = getchar();

			if (c == 'w')
			{ loc--; loc = loc>0?loc:0;choose_one(loc);continue;}
			else;
			if (c == 's')
			{ loc++; loc= loc>21?21:loc;choose_one(loc);continue;}
			else;
			if (c == 'k')
			{
				
					Package* pack = (Package* )sendBuffer;
					pack->type = '5';
					Gameconnect* con = (Gameconnect* )pack->content;
					con->type = '1';
					con->game = '1';
					char* point = scr+80*loc;
					char* cc = con->username1;
					//strcpy(con->username1,point);
					while (*point!=' ')
						{cc[0] = *point; cc++; point++;}
					con->status = '1';
					stcp_client_send(sockfd,sendBuffer,BUFFER);
				break;
			}else;
		}
			
	}else;	
	if (record == 4)
	{
		int loc = 0;
		countdown = 5;
		int t = 1;
		pthread_t threads[2];
		int rc = pthread_create(&threads[t], NULL, time_5, (void *)&t);
		if (rc){
			printf("CREATE THREAD ERROR!\n");
			exit(-1);
		}else;
		//pthread_cancel(t);
		while (1)
		{
			//fflush(stdin);
			while (!kbhit() && countdown!=0 && interface==4);
			if (interface!=4) break;else;
			if (countdown == 0)
			{
				Package* pack = (Package* )sendBuffer;
				pack->type = '4';
				Gameaction* action = (Gameaction* )pack->content;
				action->type = '1';
				action->action1 = '1'+loc;
				stcp_client_send(sockfd,sendBuffer,BUFFER);
				break;
			}else;
			char c = getchar();

			if (c == 'w')
			{ loc--; loc = loc>0?loc:0;one_playing(loc,0,0);continue;}
			else;
			if (c == 's')
			{ loc++; loc= loc>2?2:loc;one_playing(loc,0,0);continue;}
			else;
			if (c == 'k')
			{
				countdown = 0;
				Package* pack = (Package* )sendBuffer;
				pack->type = '4';
				Gameaction* action = (Gameaction* )pack->content;
				action->type = '1';
				action->action1 = '1'+loc;
				stcp_client_send(sockfd,sendBuffer,BUFFER);
				break;
				
			}else;
		}	
	}else;
	if (record == 5)
	{
		countdown = 3;
		int t = 2;
		pthread_t threads[3];
		int rc = pthread_create(&threads[t], NULL, time_3, (void *)&t);
		if (rc){
			printf("CREATE THREAD ERROR!\n");
			exit(-1);
		}else;
		
		//fflush(stdin);
		while (!kbhit()&&countdown!=0&& interface==5);
		if (interface!=5) continue;else;
		if (countdown == 0)
		{
			if (game_status == 1)
			{
				one_playing(0,0,0);
				interface = 4;
			}
			else
			{
				interface = 1;
				main_show(0);
			}
			continue;
		}else;
		getchar();

		if (game_status == 1)
		{
			one_playing(0,0,0);
			interface = 4;
		}
		else
		{
			interface = 1;
			main_show(0);
		}
		countdown = 0;
		//printf("THIS! 8");
	}else;
	if (record == 6)
	{
		int loc = 0;
		while (1)
		{

			while (!kbhit() && interface==6);
			if (interface!=6) break;else;
			char c = getchar();
			//fflush(stdin);
			if (c == 'w')
			{ loc--; loc = loc>0?loc:0;choose_chat(loc);continue;}
			else;
			if (c == 's')
			{ loc++; loc= loc>21?21:loc;choose_chat(loc);continue;}
			else;
			if (c == 'k')
			{
				printf("\033c");
				char words[50];
				printf("Please input words you want to talk\n");
				scanf("%s",words);
				//while ()
				//words[0]=getchar();
				//words[1]='\0';
					Package* pack = (Package* )sendBuffer;
					pack->type = '6';
					Talk* con = (Talk* )pack->content;
					char* point = scr+80*loc;
					char* cc = con->username;
					//strcpy(con->username1,point);
					while (*point!=' ')
						{cc[0] = *point; cc++; point++;}
				strcpy(con->content,words);
					stcp_client_send(sockfd,sendBuffer,BUFFER);
				interface = 1;
				main_show(0);
				//record == interface;
				break;
			}else;
		}
	}else;
	if (record == 7)
	{	
		int loc = 0;
		while (1)
		{
			while (!kbhit() && interface==7);
			if (interface!=7) break;else;
			char c = getchar();
			//fflush(stdin);
			if (c == 'w')
			{ loc--; loc = loc>0?loc:0;choose_two(loc);continue;}
			else;
			if (c == 's')
			{ loc++; loc= loc>21?21:loc;choose_two(loc);continue;}
			else;
			if (c == 'k')
			{
				
					Package* pack = (Package* )sendBuffer;
					pack->type = '5';
					Gameconnect* con = (Gameconnect* )pack->content;
					con->type = '1';
					con->game = '2';
					char* point = scr+80*loc;
					char* cc = con->username1;
					//strcpy(con->username1,point);
					while (*point!=' ')
						{cc[0] = *point; cc++; point++;}
					con->status = '1';
					stcp_client_send(sockfd,sendBuffer,BUFFER);
				break;
			}else;
		}
	}else;
	if (record==8)
	{
		int loc = 0;
		countdown = 5;
		int t = 1;
		pthread_t threads[2];
		int rc = pthread_create(&threads[t], NULL, time_5, (void *)&t);
		if (rc){
			printf("CREATE THREAD ERROR!\n");
			exit(-1);
		}else;
		//pthread_kill(t,SIGQUIT);
		while (1)
		{
			while (!kbhit() && countdown!=0 && interface==8);
			if (interface!=8) break;else;
			if (countdown == 0)
			{
				Package* pack = (Package* )sendBuffer;
				pack->type = '4';
				Gameaction* action = (Gameaction* )pack->content;
				action->type = '1';
				action->action1 = '1'+loc;
				stcp_client_send(sockfd,sendBuffer,BUFFER);
				break;
			}else;
			char c = getchar();
			//fflush(stdin);
			if (c == 'w')
			{ loc--; loc = loc>0?loc:0;two_playing(loc);continue;}
			else;
			if (c == 's')
			{ loc++; loc= loc>4?4:loc;two_playing(loc);continue;}
			else;
			if (c == 'k')
			{
				
				countdown = 0;
				Package* pack = (Package* )sendBuffer;
				pack->type = '4';
				Gameaction* action = (Gameaction* )pack->content;
				action->type = '1';
				action->action1 = '1'+loc;
				stcp_client_send(sockfd,sendBuffer,BUFFER);
				break;
				
			}else;
		}	
	}else;
	if (record == 9)
	{
		if (game_status == 2)
		{
			Package* pack = (Package* )sendBuffer;
				pack->type = '4';
				Gameaction* action = (Gameaction* )pack->content;
				action->type = '1';
				action->action1 = '9';
				stcp_client_send(sockfd,sendBuffer,BUFFER);
		}
		else
		{
			interface = 1;
			main_show(0);
		}
	}else;
	if (record == 10)
	{
			sleep(1);
			interface = 1;
			main_show(0);
	}else;
	if (record == 11)
	{
		while (!kbhit());
		getchar();
		interface = 1;
		main_show(0);
	}else;
	//pthead_cond_wait(&cond,&mutex);
	while (interface == record){sleep(1);printf(".");}
		//sleep(1);
		//printf("%d %d\n",interface,record);	
}
	

}


