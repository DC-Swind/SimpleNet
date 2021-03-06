#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "stcp_client.h"

/*面向应用层的接口*/

//
//  我们在下面提供了每个函数调用的原型定义和细节说明, 但这些只是指导性的, 你完全可以根据自己的想法来设计代码.
//
//  注意: 当实现这些函数时, 你需要考虑FSM中所有可能的状态, 这可以使用switch语句来实现.
//
//  目标: 你的任务就是设计并实现下面的函数原型.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// stcp客户端初始化
//
// 这个函数初始化TCB表, 将所有条目标记为NULL.  
// 它还针对重叠网络TCP套接字描述符conn初始化一个STCP层的全局变量, 该变量作为sip_sendseg和sip_recvseg的输入参数.
// 最后, 这个函数启动seghandler线程来处理进入的STCP段. 客户端只有一个seghandler.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

//client_tcb* tcb[MAX_TRANSPORT_CONNECTIONS];

void stcp_client_init(int conn) {

	int i;
	for (i=0;i<MAX_TRANSPORT_CONNECTIONS;i++)
		tcb[i] = NULL;

	tcp_conn = conn;

	pthread_t t;
	pthread_create(&t,NULL,seghandler,(void*)t);
	

  return;
}

// 创建一个客户端TCB条目, 返回套接字描述符
//
// 这个函数查找客户端TCB表以找到第一个NULL条目, 然后使用malloc()为该条目创建一个新的TCB条目.
// 该TCB中的所有字段都被初始化. 例如, TCB state被设置为CLOSED，客户端端口被设置为函数调用参数client_port. 
// TCB表中条目的索引号应作为客户端的新套接字ID被这个函数返回, 它用于标识客户端的连接. 
// 如果TCB表中没有条目可用, 这个函数返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_sock(unsigned int client_port) {

	int i =0;
	while (tcb[i]!=NULL&&i<MAX_TRANSPORT_CONNECTIONS) i++;
	if (i==MAX_TRANSPORT_CONNECTIONS)
		return -1;
	else;	

	tcb[i] = malloc(sizeof(client_tcb_t));

	tcb[i]->state = CLOSED;
	tcb[i]->client_portNum = client_port;
	tcb[i]->client_nodeID = i;
	tcb[i]->server_nodeID = 0;
	tcb[i]->server_portNum = 0;
	tcb[i]->next_seqNum = 0;
	tcb[i]->bufMutex = NULL;
	tcb[i]->sendBufHead = NULL;
	tcb[i]->sendBufunSent = NULL;
	tcb[i]->sendBufTail = NULL;
	tcb[i]->unAck_segNum = 0;

  return i;
}

// 连接STCP服务器
//
// 这个函数用于连接服务器. 它以套接字ID和服务器的端口号作为输入参数. 套接字ID用于找到TCB条目.  
// 这个函数设置TCB的服务器端口号,  然后使用sip_sendseg()发送一个SYN段给服务器.  
// 在发送了SYN段之后, 一个定时器被启动. 如果在SYNSEG_TIMEOUT时间之内没有收到SYNACK, SYN 段将被重传. 
// 如果收到了, 就返回1. 否则, 如果重传SYN的次数大于SYN_MAX_RETRY, 就将state转换到CLOSED, 并返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_connect(int sockfd, unsigned int server_port) {

	seg_t sendbuffer;
	tcb[sockfd]->server_portNum = server_port;

	sendbuffer.header.src_port = tcb[sockfd]->client_portNum;
	sendbuffer.header.dest_port = server_port;
	sendbuffer.header.type = 0;
	sendbuffer.header.length = 0;

	sip_sendseg(tcp_conn,&sendbuffer);
	tcb[sockfd]->state = SYNSENT;

	printf("send syn successful\n");

	int count = 1;
	while (1)
	{
		usleep(100000);
		
		if (tcb[sockfd]->state == CONNECTED)
			return 1;
		if (count<SYN_MAX_RETRY)
		{
			sip_sendseg(tcp_conn,&sendbuffer);
			printf("send syn again\n");
			count++;
		}
		else
		{
			tcb[sockfd]->state = CLOSED;
			printf("send syn over max\n");
			return -1;
		}
		
	}

  return 0;
}

// 发送数据给STCP服务器
//
// 这个函数发送数据给STCP服务器. 你不需要在本实验中实现它。
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_send(int sockfd, void* data, unsigned int length) {

	return 1;
}

// 断开到STCP服务器的连接
//
// 这个函数用于断开到服务器的连接. 它以套接字ID作为输入参数. 套接字ID用于找到TCB表中的条目.  
// 这个函数发送FIN segment给服务器. 在发送FIN之后, state将转换到FINWAIT, 并启动一个定时器.
// 如果在最终超时之前state转换到CLOSED, 则表明FINACK已被成功接收. 否则, 如果在经过FIN_MAX_RETRY次尝试之后,
// state仍然为FINWAIT, state将转换到CLOSED, 并返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_disconnect(int sockfd) {

	seg_t sendbuffer;

	sendbuffer.header.src_port = tcb[sockfd]->client_portNum;
	sendbuffer.header.dest_port = tcb[sockfd]->server_portNum;
	sendbuffer.header.type = 2;
	sendbuffer.header.length = 0;

	sip_sendseg(tcp_conn,&sendbuffer);
	printf("send fin successful\n");
	tcb[sockfd]->state = FINWAIT;
	
	int count = 1;
	while (1)
	{
		usleep(100000);
		
		if (tcb[sockfd]->state == CLOSED)
			return 1;
		if (count<FIN_MAX_RETRY)
		{
			sip_sendseg(tcp_conn,&sendbuffer);
			printf("send fin again\n");
			count++;
		}
		else
		{
			tcb[sockfd]->state = CLOSED;
			printf("send fin over max\n");
			return -1;
		}
		
	}
	

  return 0;
}

// 关闭STCP客户
//
// 这个函数调用free()释放TCB条目. 它将该条目标记为NULL, 成功时(即位于正确的状态)返回1,
// 失败时(即位于错误的状态)返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_close(int sockfd) {
	if (tcb[sockfd] != NULL)
	{
		free(tcb[sockfd]);
		tcb[sockfd] = NULL;
		return 1;
	}
	else
		return -1;
	return 0;
}

// 处理进入段的线程
//
// 这是由stcp_client_init()启动的线程. 它处理所有来自服务器的进入段. 
// seghandler被设计为一个调用sip_recvseg()的无穷循环. 如果sip_recvseg()失败, 则说明重叠网络连接已关闭,
// 线程将终止. 根据STCP段到达时连接所处的状态, 可以采取不同的动作. 请查看客户端FSM以了解更多细节.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

void handle(seg_t* buffer);

void *seghandler(void* arg) {

	seg_t recvbuffer;
	while(1)
	{
		int temp = sip_recvseg(tcp_conn,&recvbuffer);
	if (temp == 0)
	{
		//printf("recv buffer\n");
		handle(&recvbuffer);
	}else
	{
		if (temp == 2)
		{
			printf("RECV ERROR!\n");
			exit(0);
		}else;
	}
	}

  return 0;
}

void handle(seg_t* buffer)
{
	short int type = buffer->header.type;
	int i=0;
	for (i=0;i<MAX_TRANSPORT_CONNECTIONS;i++)
	{
		if (tcb[i]==NULL)
			continue;
		else;
		if (buffer->header.dest_port == tcb[i]->client_portNum)
			break;
		else;
	}
	if (i==MAX_TRANSPORT_CONNECTIONS)
		return;

	//printf("recv buffer type = %d\n",type);

	switch (type)
	{
	case 1:
		printf("recv SYN_ACK\n");
		print_stcphdr(&buffer->header);
		if (tcb[i]->state==SYNSENT)
		{
			tcb[i]->state=CONNECTED;
			return;
		}else
			return;
	case 3:
		printf("recv FIN_ACK\n");
		print_stcphdr(&buffer->header);
		if (tcb[i]->state==FINWAIT)
		{
			tcb[i]->state=CLOSED;
			return;
		}else
			return;
	}
}



