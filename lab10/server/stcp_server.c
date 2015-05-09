//�ļ���: server/stcp_server.c
//
//����: ����ļ�����STCP�������ӿ�ʵ��. 
//
//��������: 2015��

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/select.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include "stcp_server.h"
#include "../topology/topology.h"
#include "../common/constants.h"
#include "../common/seg.h"

//����tcbtableΪȫ�ֱ���
//server_tcb_t* tcbtable[MAX_TRANSPORT_CONNECTIONS];
server_tcb_t* TCB[MAX_TRANSPORT_CONNECTIONS];
//������SIP���̵�����Ϊȫ�ֱ���
int sip_conn;
char ds[][10] = {"SYN","SYNACK","FIN","FINACK","DATA","DATAACK"};

int portToindex(int port){
    int i;
    for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++) 
        if (TCB[i] != NULL && TCB[i]->server_portNum == port) return i;
    return -1;
}
void *closewait(void* index){
    int i = *((int*)index);
    if (index != NULL) free(index);
    sleep(CLOSEWAIT_TIMEOUT);
    TCB[i]->state = CLOSED;
    return NULL;
}
int stcp_server_closewait(int index){
    int* i = malloc(sizeof(int));
    *i = index;
    if (TCB[index]->state != CLOSEWAIT) return -1;
    pthread_t close;
    pthread_create(&close,NULL,closewait,(void*)i);

    return 0;
}
int sockfdValid(int sockfd){
    if (sockfd < 0 || sockfd >= MAX_TRANSPORT_CONNECTIONS) return 0;
    if (TCB[sockfd] == NULL) return 0;
    return 1;
}

/*********************************************************************/
//
//STCP APIʵ��
//
/*********************************************************************/

// ���������ʼ��TCB��, ��������Ŀ���ΪNULL. �������TCP�׽���������conn��ʼ��һ��STCP���ȫ�ֱ���, 
// �ñ�����Ϊsip_sendseg��sip_recvseg���������. ���, �����������seghandler�߳�����������STCP��.
// ������ֻ��һ��seghandler.
void stcp_server_init(int conn) 
{
    int i;
    for (i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) TCB[i] = NULL;
    sip_conn = conn;
    
    pthread_t handler;
    pthread_create(&handler,NULL,seghandler,NULL);
    printf("stcp_server_init() success.\n");
    return;
}

// ����������ҷ�����TCB�����ҵ���һ��NULL��Ŀ, Ȼ��ʹ��malloc()Ϊ����Ŀ����һ���µ�TCB��Ŀ.
// ��TCB�е������ֶζ�����ʼ��, ����, TCB state������ΪCLOSED, �������˿ڱ�����Ϊ�������ò���server_port. 
// TCB������Ŀ������Ӧ��Ϊ�����������׽���ID�������������, �����ڱ�ʶ�������˵�����. 
// ���TCB����û����Ŀ����, �����������-1.
int stcp_server_sock(unsigned int server_port) 
{
	int i,rt = -1;
    server_tcb_t *tcb;
    for (i=0; i<MAX_TRANSPORT_CONNECTIONS;i++) if(TCB[i] == NULL){
        TCB[i] = malloc(sizeof(server_tcb_t));
        tcb = TCB[i];
        rt = i;
        break;
    }
    
    tcb->server_nodeID = topology_getMyNodeID();
    tcb->server_portNum = server_port;
    tcb->client_nodeID = 0;
    tcb->client_portNum = 0;
    tcb->state = CLOSED;
    tcb->expect_seqNum = 0; 
    tcb->recvBuf = (char*)malloc(RECEIVE_BUF_SIZE);
    tcb->usedBufLen = 0;
    tcb->bufMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(tcb->bufMutex,NULL);
    tcb->sendbufMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(tcb->sendbufMutex,NULL);

    tcb->sendBufHead = tcb->sendBufunSent = tcb->sendBufTail = NULL;
    return rt;
}

// �������ʹ��sockfd���TCBָ��, �������ӵ�stateת��ΪLISTENING. ��Ȼ��������ʱ������æ�ȴ�ֱ��TCB״̬ת��ΪCONNECTED 
// (���յ�SYNʱ, seghandler�����״̬��ת��). �ú�����һ������ѭ���еȴ�TCB��stateת��ΪCONNECTED,  
// ��������ת��ʱ, �ú�������1. �����ʹ�ò�ͬ�ķ�����ʵ�����������ȴ�.
int stcp_server_accept(int sockfd) 
{
    if (!sockfdValid(sockfd)) return -1;
    server_tcb_t* tcb = TCB[sockfd];
    tcb->state = LISTENING;
    while(tcb->state != CONNECTED){
        usleep(50000);
    }
    printf("A client connect success!\n");
    return 0;

}

int stcp_server_send(int sockfd, void* data, unsigned int length){
    if (!sockfdValid(sockfd)) return -1;
    if (TCB[sockfd]->state != CONNECTED) return -1;
    //��Ƭ
    char* p = (char*)data;
    int least = (length%MAX_SEG_LEN > 0)?1:0;
    int i,totalseg = length/MAX_SEG_LEN + least;
    pthread_mutex_lock(TCB[sockfd]->sendbufMutex);
    for(i=0; i<totalseg; i++){
        //create seg_t
        //seg_t sendsegspace;
        //seg_t* sendseg = &sendsegspace;
        seg_t* sendseg = (seg_t*)malloc(sizeof(seg_t));
        sendseg->header.src_port = TCB[sockfd]->server_portNum;
        sendseg->header.dest_port = TCB[sockfd]->client_portNum;
        sendseg->header.seq_num = TCB[sockfd]->next_seqNum;
        TCB[sockfd]->next_seqNum = (TCB[sockfd]->next_seqNum + 1)%MAX_SEGNUM;
        sendseg->header.ack_num = 0;
        sendseg->header.type = DATA;
        sendseg->header.rcv_win = 0;
        sendseg->header.checksum = 0;
        if (i == totalseg -1 && least == 1){
            memcpy(sendseg->data,&p[i*MAX_SEG_LEN],length%MAX_SEG_LEN);
            sendseg->header.length = length % MAX_SEG_LEN;
        }else{ 
            memcpy(sendseg->data,&p[i*MAX_SEG_LEN],MAX_SEG_LEN);
            sendseg->header.length = MAX_SEG_LEN;
        }
        //insert seg_t to segBuf_t
        if (TCB[sockfd]->sendBufHead == NULL){
            TCB[sockfd]->sendBufHead = (segBuf_t*)malloc(sizeof(segBuf_t));
            TCB[sockfd]->sendBufHead->next = NULL;
            memcpy((char*)&TCB[sockfd]->sendBufHead->seg,(char*)sendseg,sizeof(seg_t));
            TCB[sockfd]->sendBufTail = TCB[sockfd]->sendBufHead;
            TCB[sockfd]->sendBufunSent = TCB[sockfd]->sendBufHead;
            pthread_t timer;
            int *socket = malloc(sizeof(int));
            *socket = sockfd;
            pthread_create(&timer,NULL,sendBuf_timer,(void*)socket);
        }else{
            TCB[sockfd]->sendBufTail->next = (segBuf_t*)malloc(sizeof(segBuf_t));
            TCB[sockfd]->sendBufTail->next->next = NULL;
            memcpy((char*)&TCB[sockfd]->sendBufTail->next->seg,(char*)sendseg,sizeof(seg_t));
            TCB[sockfd]->sendBufTail = TCB[sockfd]->sendBufTail->next;
            if (TCB[sockfd]->sendBufunSent == NULL) 
                TCB[sockfd]->sendBufunSent = TCB[sockfd]->sendBufTail;
        }
        
        if (TCB[sockfd]->unAck_segNum < GBN_WINDOW){
            sip_sendseg(sip_conn,TCB[sockfd]->client_nodeID,&TCB[sockfd]->sendBufunSent->seg);
            TCB[sockfd]->unAck_segNum++;
            struct timeval tv;
            gettimeofday(&tv,NULL);
            TCB[sockfd]->sendBufunSent->sentTime = (unsigned int)tv.tv_usec;
            TCB[sockfd]->sendBufunSent = TCB[sockfd]->sendBufunSent->next;
        }
    }
    pthread_mutex_unlock(TCB[sockfd]->sendbufMutex); 
    return 1;
}

// ��������STCP�ͻ��˵�����. �������ÿ��RECVBUF_POLLING_INTERVALʱ��
// �Ͳ�ѯ���ջ�����, ֱ���ȴ������ݵ���, ��Ȼ��洢���ݲ�����1. ����������ʧ��, �򷵻�-1.
int stcp_server_recv(int sockfd, void* buf, unsigned int length) 
{
	//������sockfdʹ��ǰ�ж�sockfd�Ƿ���Ч��
    if (!sockfdValid(sockfd)) return -1;
    if (TCB[sockfd]->state != CONNECTED) return -1;
    while(1){
        pthread_mutex_lock(TCB[sockfd]->bufMutex);
        if(length > TCB[sockfd]->usedBufLen){

        }else{
            memcpy(buf,TCB[sockfd]->recvBuf,length);
            TCB[sockfd]->usedBufLen -= length;
            memcpy(TCB[sockfd]->recvBuf,&TCB[sockfd]->recvBuf[length],TCB[sockfd]->usedBufLen);
            pthread_mutex_unlock(TCB[sockfd]->bufMutex);
            return 1;
        } 
        pthread_mutex_unlock(TCB[sockfd]->bufMutex);
        sleep(RECVBUF_POLLING_INTERVAL);
    }
    return -1;

}

// �����������free()�ͷ�TCB��Ŀ. ��������Ŀ���ΪNULL, �ɹ�ʱ(��λ����ȷ��״̬)����1,
// ʧ��ʱ(��λ�ڴ����״̬)����-1.
int stcp_server_close(int sockfd) 
{
	//�ǵ�free������malloc�Ŀռ�
    if (!sockfdValid(sockfd)) return -1;
    int index = sockfd;
    if (TCB[index]->state == CLOSED){ 
        pthread_mutex_t* mutex = TCB[index]->bufMutex;
        pthread_mutex_lock(mutex);
        if (TCB[index]->recvBuf != NULL) free(TCB[index]->recvBuf);
        if (TCB[index] != NULL)free(TCB[index]);
        TCB[index] = NULL;
        pthread_mutex_unlock(mutex);
        return 1;
    }else{
        return -1;
    }

}

// ������stcp_server_init()�������߳�. �������������Կͻ��˵Ľ�������. seghandler�����Ϊһ������sip_recvseg()������ѭ��, 
// ���sip_recvseg()ʧ��, ��˵����SIP���̵������ѹر�, �߳̽���ֹ. ����STCP�ε���ʱ����������״̬, ���Բ�ȡ��ͬ�Ķ���.
// ��鿴�����FSM���˽����ϸ��.
void* seghandler(void* arg) 
{
	seg_t* buffer = (seg_t*)malloc(MAX_SEG_LEN + HDR_LEN);
    while(1){
        char sbuffer[MAX_SEG_LEN + HDR_LEN];
        seg_t* sbuf = (seg_t*)sbuffer;
        int src_nodeID;
        int sendrt = sip_recvseg(sip_conn,&src_nodeID,buffer);
        if (sendrt == -1){
            //error , seg missing...
            printf("sip_recvseg error in seghandler.\n");
            usleep(10000);
            //sleep(1);
            continue;
        }else if (sendrt == 2){
            printf("client TCP disconnect!\n");
            break;
        }
       
        stcp_hdr_t *tcphdr = (stcp_hdr_t *)buffer;
        stcp_hdr_t *shdr = (stcp_hdr_t *)sbuf;
        int index = portToindex(tcphdr->dest_port);
        printf("recv a %s. from nodeID:%d\n",ds[tcphdr->type],src_nodeID);
        print_stcphdr(tcphdr);
        if (index == -1){ 
            printf("there is no this port\n");
            continue;
        }
        switch(tcphdr->type){
            case SYN: 
                TCB[index]->client_nodeID = src_nodeID;
                if (TCB[index]->state == LISTENING || TCB[index]->state == CONNECTED){
                    shdr->src_port = tcphdr->dest_port;
                    shdr->dest_port = tcphdr->src_port;
                    shdr->seq_num = 0;
                    shdr->ack_num = 0;
                    shdr->length = 0;
                    shdr->type = SYNACK;
                    shdr->rcv_win = 0;
                    shdr->checksum = 0;
                    if (sip_sendseg(sip_conn,TCB[index]->client_nodeID,sbuf) > 0){
                        printf("send SYNACK to client:%d success!nodeID is %d.\n",shdr->dest_port,TCB[index]->client_nodeID);
                        TCB[index]->state = CONNECTED;
                        TCB[index]->client_portNum = shdr->dest_port;
                    }else{
                        //stcp_server_close(tcphdr->dest_port);
                        printf("stcp socket %d disconnected.error\n",tcphdr->dest_port);
                    }
                }
                break;
            case SYNACK:
                break;
            case FIN:
                if (TCB[index]->state == CONNECTED || TCB[index]->state == CLOSEWAIT){
                    shdr->src_port = tcphdr->dest_port;
                    shdr->dest_port = tcphdr->src_port;
                    shdr->seq_num = 0;
                    shdr->ack_num = 0;
                    shdr->length = 0;
                    shdr->type = FINACK;
                    shdr->rcv_win = 0;
                    shdr->checksum = 0;
                    if (sip_sendseg(sip_conn,TCB[index]->client_nodeID,sbuf) > 0){
                        TCB[index]->state = CLOSEWAIT;
                        stcp_server_closewait(index);
                    }else{
                        //stcp_server_close(tcphdr->dest_port);
                        printf("stcp socket %d disconnected.error\n",tcphdr->dest_port);
                    }
                }
            case FINACK:
                break;
            case DATA:
                if (TCB[index]->state == CONNECTED && TCB[index]->usedBufLen + tcphdr->length <= RECEIVE_BUF_SIZE){
                    if (TCB[index]->expect_seqNum < tcphdr->seq_num)break;
                    if (TCB[index]->expect_seqNum == tcphdr->seq_num){
                        pthread_mutex_lock(TCB[index]->bufMutex);
                        memcpy(&TCB[index]->recvBuf[TCB[index]->usedBufLen],buffer->data,tcphdr->length);
                        TCB[index]->usedBufLen += tcphdr->length;
                        printf("already receive %d.\n",TCB[index]->usedBufLen);
                        TCB[index]->expect_seqNum += 1;
                        pthread_mutex_unlock(TCB[index]->bufMutex);
                    }else{

                    } 
                    shdr->src_port = tcphdr->dest_port;
                    shdr->dest_port = tcphdr->src_port;
                    shdr->seq_num = 0;
                    shdr->ack_num = TCB[index]->expect_seqNum ; //-1 or not
                    shdr->length = 0;
                    shdr->type = DATAACK;
                    shdr->rcv_win =0;
                    shdr->checksum = 0;
                    if (sip_sendseg(sip_conn,TCB[index]->client_nodeID,sbuf) > 0){
                        printf("send DATAACK to socket:%d.\n",index);
                    }else{
                        printf("stcp send DATAACK failed to socket:%d.\n",index);
                    }
                }
                break;
            case DATAACK:
                if (TCB[index]->state == CONNECTED){
                    pthread_mutex_lock(TCB[index]->sendbufMutex); 
                    segBuf_t* p = TCB[index]->sendBufHead;
                    segBuf_t* pp;
                    while(p!= NULL && p!= TCB[index]->sendBufunSent && p->seg.header.seq_num < tcphdr->ack_num){
                        pp = p;
                        p = p->next;
                        free(pp);
                        TCB[index]->unAck_segNum--;
                    } 
                    TCB[index]->sendBufHead = p;
                    while(TCB[index]->sendBufunSent!=NULL && TCB[index]->unAck_segNum < GBN_WINDOW){
                        sip_sendseg(sip_conn,TCB[index]->client_nodeID,&TCB[index]->sendBufunSent->seg);
                        struct timeval tv;
                        gettimeofday(&tv,NULL);
                        TCB[index]->sendBufunSent->sentTime = (unsigned int)tv.tv_usec;
                        TCB[index]->sendBufunSent = TCB[index]->sendBufunSent->next;
                        TCB[index]->unAck_segNum = (TCB[index]->unAck_segNum + 1)%MAX_SEGNUM;
                    }
                    pthread_mutex_unlock(TCB[index]->sendbufMutex);
                }
                break;
            default: break;
        }        

    }
    free(buffer);
    printf("expect seqnum is %d\n",TCB[0]->expect_seqNum);
    printf("seghandler exit.\n");
    return 0;

}

void* sendBuf_timer(void* clienttcb){
    int sockfd = *((int*)clienttcb);
    while(1){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        pthread_mutex_lock(TCB[sockfd]->sendbufMutex);
        if (TCB[sockfd]->sendBufHead == NULL){
            pthread_mutex_unlock(TCB[sockfd]->sendbufMutex);
            return NULL;
        }
        if(((unsigned int)tv.tv_usec)-TCB[sockfd]->sendBufHead->sentTime>DATA_TIMEOUT/1000){
            segBuf_t* p = TCB[sockfd]->sendBufHead;
            while(p != TCB[sockfd]->sendBufunSent){
                sip_sendseg(sip_conn,TCB[sockfd]->client_nodeID,&p->seg);
                struct timeval tv;
                gettimeofday(&tv,NULL);
                p->sentTime = (unsigned int)tv.tv_usec;
                p = p->next;
            }
        }
        pthread_mutex_unlock(TCB[sockfd]->sendbufMutex);    
        usleep(SENDBUF_POLLING_INTERVAL/1000);
    }
    return NULL; 
}

// ����̳߳�����ѯ���ͻ������Դ�����ʱ�¼�. ������ͻ������ǿ�, ��Ӧһֱ����.
// ���(��ǰʱ�� - ��һ���ѷ��͵�δ��ȷ�϶εķ���ʱ��) > DATA_TIMEOUT, �ͷ���һ�γ�ʱ�¼�.
// ����ʱ�¼�����ʱ, ���·��������ѷ��͵�δ��ȷ�϶�. �����ͻ�����Ϊ��ʱ, ����߳̽���ֹ.