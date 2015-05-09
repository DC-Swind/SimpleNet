//�ļ���: client/stcp_client.c
//
//����: ����ļ�����STCP�ͻ��˽ӿ�ʵ�� 
//
//��������: 2015��

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <assert.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "../topology/topology.h"
#include "stcp_client.h"
#include "../common/seg.h"

//����tcbtableΪȫ�ֱ���
client_tcb_t* tcb[MAX_TRANSPORT_CONNECTIONS];
//������SIP���̵�TCP����Ϊȫ�ֱ���
int sip_conn;

/*********************************************************************/
//
//STCP APIʵ��
//
/*********************************************************************/

// ���������ʼ��TCB��, ��������Ŀ���ΪNULL.  
// �������TCP�׽���������conn��ʼ��һ��STCP���ȫ�ֱ���, �ñ�����Ϊsip_sendseg��sip_recvseg���������.
// ���, �����������seghandler�߳�����������STCP��. �ͻ���ֻ��һ��seghandler.
void stcp_client_init(int conn) 
{
    int i;
    for (i=0;i<MAX_TRANSPORT_CONNECTIONS;i++)
        tcb[i] = NULL;
    sip_conn = conn;

    pthread_t t;
    pthread_create(&t,NULL,seghandler,(void*)t);
    return;
}

// ����������ҿͻ���TCB�����ҵ���һ��NULL��Ŀ, Ȼ��ʹ��malloc()Ϊ����Ŀ����һ���µ�TCB��Ŀ.
// ��TCB�е������ֶζ�����ʼ��. ����, TCB state������ΪCLOSED���ͻ��˶˿ڱ�����Ϊ�������ò���client_port. 
// TCB������Ŀ��������Ӧ��Ϊ�ͻ��˵����׽���ID�������������, �����ڱ�ʶ�ͻ��˵�����. 
// ���TCB����û����Ŀ����, �����������-1.
int stcp_client_sock(unsigned int client_port) 
{
    int i =0;
    while (tcb[i]!=NULL&&i<MAX_TRANSPORT_CONNECTIONS) i++;
    if (i==MAX_TRANSPORT_CONNECTIONS)
        return -1;
    else;   

    tcb[i] = malloc(sizeof(client_tcb_t));

    tcb[i]->state = CLOSED;
    tcb[i]->client_portNum = client_port;
    tcb[i]->client_nodeID = topology_getMyNodeID();
    tcb[i]->server_nodeID = 0;
    tcb[i]->server_portNum = 0;
    tcb[i]->next_seqNum = 0;
    tcb[i]->bufMutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(tcb[i]->bufMutex,NULL);
    tcb[i]->sendBufHead = NULL;
    tcb[i]->sendBufunSent = NULL;
    tcb[i]->sendBufTail = NULL;
    tcb[i]->unAck_segNum = 0;
    tcb[i]->sendbufMutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(tcb[i]->sendbufMutex,NULL);
	return i;
}

// ��������������ӷ�����. �����׽���ID, �������ڵ�ID�ͷ������Ķ˿ں���Ϊ�������. �׽���ID�����ҵ�TCB��Ŀ.  
// �����������TCB�ķ������ڵ�ID�ͷ������˿ں�,  Ȼ��ʹ��sip_sendseg()����һ��SYN�θ�������.  
// �ڷ�����SYN��֮��, һ����ʱ��������. �����SYNSEG_TIMEOUTʱ��֮��û���յ�SYNACK, SYN �ν����ش�. 
// ����յ���, �ͷ���1. ����, ����ش�SYN�Ĵ�������SYN_MAX_RETRY, �ͽ�stateת����CLOSED, ������-1.
int stcp_client_connect(int sockfd, int nodeID, unsigned int server_port) 
{
    seg_t sendbuffer;
    tcb[sockfd]->server_nodeID = nodeID;
    tcb[sockfd]->server_portNum = server_port;

    sendbuffer.header.src_port = tcb[sockfd]->client_portNum;
    sendbuffer.header.dest_port = server_port;
    sendbuffer.header.type = 0;
    sendbuffer.header.length = 0;

    if (sip_sendseg(sip_conn,nodeID,&sendbuffer) == 1){
        printf("send syn successful to nodeID(%d) in stcp_client_connect.\n",nodeID);
    }else{
        printf("send syn error in stcp_client_connect.\n");
    }
    tcb[sockfd]->state = SYNSENT;

    

    int count = 1;
    while (1)
    {
        usleep(100000);
        
        if (tcb[sockfd]->state == CONNECTED)
            return 1;
        if (count<SYN_MAX_RETRY)
        {
            if (sip_sendseg(sip_conn,nodeID,&sendbuffer) == 1){
                printf("send syn again success.\n");
            }else{
                printf("send syn again error.\n");
            }
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

// �������ݸ�STCP������. �������ʹ���׽���ID�ҵ�TCB���е���Ŀ.
// Ȼ����ʹ���ṩ�����ݴ���segBuf, �������ӵ����ͻ�����������.
// ������ͻ������ڲ�������֮ǰΪ��, һ����Ϊsendbuf_timer���߳̾ͻ�����.
// ÿ��SENDBUF_ROLLING_INTERVALʱ���ѯ���ͻ������Լ���Ƿ��г�ʱ�¼�����. 
// ��������ڳɹ�ʱ����1�����򷵻�-1. 
// stcp_client_send��һ����������������.
// ��Ϊ�û����ݱ���ƬΪ�̶���С��STCP��, ����һ��stcp_client_send���ÿ��ܻ�������segBuf
// ����ӵ����ͻ�����������. ������óɹ�, ���ݾͱ�����TCB���ͻ�����������, ���ݻ������ڵ����,
// ���ݿ��ܱ����䵽������, ���ڶ����еȴ�����.
int stcp_client_send(int sockfd, void* data, unsigned int length) 
{
    if (tcb[sockfd]->state != CONNECTED)
        return -1;
    else;
    //printf("-1\n");
    char* p = (char*)data;
    int least = (length%MAX_SEG_LEN > 0)?1:0;
    int i,totalseg = length/MAX_SEG_LEN + least;
    pthread_mutex_lock(tcb[sockfd]->sendbufMutex);
    for(i=0; i<totalseg; i++){
        //create seg_t
    //printf("0\n");
        seg_t* sendseg = (seg_t*)malloc(sizeof(seg_t));
        sendseg->header.src_port = tcb[sockfd]->client_portNum;
        sendseg->header.dest_port = tcb[sockfd]->server_portNum;
        sendseg->header.seq_num = tcb[sockfd]->next_seqNum;
        tcb[sockfd]->next_seqNum = (tcb[sockfd]->next_seqNum + 1)%MAX_SEGNUM;
        sendseg->header.ack_num = 0;
        sendseg->header.type = DATA;
        sendseg->header.rcv_win = 0;
        sendseg->header.checksum = 0;
    //printf("1\n");
        if (i == totalseg -1 && least == 1){
            memcpy(sendseg->data,&p[i*MAX_SEG_LEN],length%MAX_SEG_LEN);
            sendseg->header.length = length % MAX_SEG_LEN;
        }else{ 
            memcpy(sendseg->data,&p[i*MAX_SEG_LEN],MAX_SEG_LEN);
            sendseg->header.length = MAX_SEG_LEN;
        }
        //insert seg_t to segBuf_t
    //printf("2\n");
        if (tcb[sockfd]->sendBufHead == NULL){
            tcb[sockfd]->sendBufHead = (segBuf_t*)malloc(sizeof(segBuf_t));
            tcb[sockfd]->sendBufHead->next = NULL;
            memcpy((char*)&tcb[sockfd]->sendBufHead->seg,(char*)sendseg,sizeof(seg_t));
            tcb[sockfd]->sendBufTail = tcb[sockfd]->sendBufHead;
            tcb[sockfd]->sendBufunSent = tcb[sockfd]->sendBufHead;
            pthread_t timer;
            int* socket = malloc(4);
        *socket = sockfd;
            pthread_create(&timer,NULL,sendBuf_timer,(void* )socket);
        }else{
            tcb[sockfd]->sendBufTail->next = (segBuf_t*)malloc(sizeof(segBuf_t));
            tcb[sockfd]->sendBufTail->next->next = NULL;
            memcpy((char*)&tcb[sockfd]->sendBufTail->next->seg,(char*)sendseg,sizeof(seg_t));
            tcb[sockfd]->sendBufTail = tcb[sockfd]->sendBufTail->next;
            if (tcb[sockfd]->sendBufunSent == NULL) 
                tcb[sockfd]->sendBufunSent = tcb[sockfd]->sendBufTail;
        }
        //printf("3\n");
    //print_stcphdr(&sendseg->header);
        if (tcb[sockfd]->unAck_segNum < GBN_WINDOW){
        //print_stcphdr(&tcb[sockfd]->sendBufunSent->seg.header);
            sip_sendseg(sip_conn,tcb[sockfd]->server_nodeID,&tcb[sockfd]->sendBufunSent->seg);
            tcb[sockfd]->unAck_segNum++;
            struct timeval tv;
            gettimeofday(&tv,NULL);
            tcb[sockfd]->sendBufunSent->sentTime = (unsigned int)tv.tv_usec;
            tcb[sockfd]->sendBufunSent = tcb[sockfd]->sendBufunSent->next;
        }
    }
    pthread_mutex_unlock(tcb[sockfd]->sendbufMutex);    

    return 1;

}

int stcp_client_recv(int sockfd, void* buf, unsigned int length)
{
    if (tcb[sockfd]->state != CONNECTED) return -1;
    while(1){
        pthread_mutex_lock(tcb[sockfd]->bufMutex);
        if(length > tcb[sockfd]->usedBufLen){

        }else{
            strncpy(buf,tcb[sockfd]->recvBuf,length);
            strncpy(tcb[sockfd]->recvBuf,&tcb[sockfd]->recvBuf[length],length);
            tcb[sockfd]->usedBufLen -= length;
            pthread_mutex_unlock(tcb[sockfd]->bufMutex);
            return 1;
        } 
        pthread_mutex_unlock(tcb[sockfd]->bufMutex);
        sleep(RECVBUF_POLLING_INTERVAL);
    }
    return -1;
}


// ����������ڶϿ���������������. �����׽���ID��Ϊ�������. �׽���ID�����ҵ�TCB���е���Ŀ.  
// �����������FIN�θ�������. �ڷ���FIN֮��, state��ת����FINWAIT, ������һ����ʱ��.
// ��������ճ�ʱ֮ǰstateת����CLOSED, �����FINACK�ѱ��ɹ�����. ����, ����ھ���FIN_MAX_RETRY�γ���֮��,
// state��ȻΪFINWAIT, state��ת����CLOSED, ������-1.
int stcp_client_disconnect(int sockfd) 
{
    seg_t sendbuffer;

    sendbuffer.header.src_port = tcb[sockfd]->client_portNum;
    sendbuffer.header.dest_port = tcb[sockfd]->server_portNum;
    sendbuffer.header.type = 2;
    sendbuffer.header.length = 0;

    sip_sendseg(sip_conn,tcb[sockfd]->server_nodeID,&sendbuffer);
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
            sip_sendseg(sip_conn,tcb[sockfd]->server_nodeID,&sendbuffer);
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

// �����������free()�ͷ�TCB��Ŀ. ��������Ŀ���ΪNULL, �ɹ�ʱ(��λ����ȷ��״̬)����1,
// ʧ��ʱ(��λ�ڴ����״̬)����-1.
int stcp_client_close(int sockfd) 
{
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

// ������stcp_client_init()�������߳�. �������������Է������Ľ����. 
// seghandler�����Ϊһ������sip_recvseg()������ѭ��. ���sip_recvseg()ʧ��, ��˵����SIP���̵������ѹر�,
// �߳̽���ֹ. ����STCP�ε���ʱ����������״̬, ���Բ�ȡ��ͬ�Ķ���. ��鿴�ͻ���FSM���˽����ϸ��.
void handle(seg_t* buffer,int src_nodeID);
void* seghandler(void* arg) 
{
    seg_t recvbuffer;
    while(1)
    {
        int src_nodeID;
        int temp = sip_recvseg(sip_conn,&src_nodeID,&recvbuffer);
    if (temp == 1)
    {
        //printf("recv buffer\n");
        handle(&recvbuffer,src_nodeID);
    }else
    {
        if (temp == 2)
        {
            printf("RECV ERROR!\n");
            exit(0);
        }else;
    }
    }

    return NULL;
}

void handle(seg_t* buffer,int src_nodeID)
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
        printf("recv SYN_ACK from nodeID:%d\n",src_nodeID);
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
    case 4:
        printf("recv DATA\n");
        print_stcphdr(&buffer->header);
        if (tcb[i]->state==CONNECTED)
        {
            return;
        }else
            return;
    case 5:
        printf("recv DATAACK\n");
        print_stcphdr(&buffer->header);
        if (tcb[i]->state==CONNECTED)
        {
            pthread_mutex_lock(tcb[i]->sendbufMutex); 
            segBuf_t* p = tcb[i]->sendBufHead;
            while(p!= NULL && p!= tcb[i]->sendBufunSent && p->seg.header.seq_num < buffer->header.ack_num){
                p = p->next;
                tcb[i]->unAck_segNum--;
                                printf("acknum is %d,unack is %d.\n",buffer->header.ack_num,tcb[i]->unAck_segNum);
            }
            tcb[i]->sendBufHead = p;
                    while(tcb[i]->sendBufunSent!=NULL && tcb[i]->unAck_segNum < GBN_WINDOW){
                        sip_sendseg(sip_conn,tcb[i]->server_nodeID,&tcb[i]->sendBufunSent->seg);
                        struct timeval tv;
                        gettimeofday(&tv,NULL);
                        tcb[i]->sendBufunSent->sentTime = (unsigned int)tv.tv_usec;
                        tcb[i]->sendBufunSent = tcb[i]->sendBufunSent->next;
                        tcb[i]->unAck_segNum = (tcb[i]->unAck_segNum + 1)%MAX_SEGNUM;
                    }
                    pthread_mutex_unlock(tcb[i]->sendbufMutex);
        }else
            return;
    }
}


//����̳߳�����ѯ���ͻ������Դ�����ʱ�¼�. ������ͻ������ǿ�, ��Ӧһֱ����.
//���(��ǰʱ�� - ��һ���ѷ��͵�δ��ȷ�϶εķ���ʱ��) > DATA_TIMEOUT, �ͷ���һ�γ�ʱ�¼�.
//����ʱ�¼�����ʱ, ���·��������ѷ��͵�δ��ȷ�϶�. �����ͻ�����Ϊ��ʱ, ����߳̽���ֹ.
void* sendBuf_timer(void* clienttcb) 
{
    int sockfd = *((int*)clienttcb);
    while(1){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        pthread_mutex_lock(tcb[sockfd]->sendbufMutex);
        if (tcb[sockfd]->sendBufHead == NULL){
            pthread_mutex_unlock(tcb[sockfd]->sendbufMutex);
            return NULL;
        }
    //printf("%d %d %d\n",tv.tv_usec,tcb[sockfd]->sendBufHead->sentTime,DATA_TIMEOUT/1000);
        if(((unsigned int)tv.tv_usec)-tcb[sockfd]->sendBufHead->sentTime>DATA_TIMEOUT/1000){
            segBuf_t* p = tcb[sockfd]->sendBufHead;
            while(p != tcb[sockfd]->sendBufunSent){
                sip_sendseg(sip_conn,tcb[sockfd]->server_nodeID,&p->seg);
                struct timeval tv;
                gettimeofday(&tv,NULL);
                p->sentTime = (unsigned int)tv.tv_usec;
                p = p->next;
            }
        }
        pthread_mutex_unlock(tcb[sockfd]->sendbufMutex);    
        usleep(SENDBUF_POLLING_INTERVAL/1000);
    }

	return NULL;
}

