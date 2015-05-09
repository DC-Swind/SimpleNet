// 文件名: stcp_server.c
//
// 描述: 这个文件包含服务器STCP套接字接口定义. 你需要实现所有这些接口.
//
// 创建日期: 2015年
//

#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "stcp_server.h"
#include "../common/constants.h"

server_tcb_t* TCB[MAX_TRANSPORT_CONNECTIONS];
int son_conn;
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
//
//  用于服务器程序的STCP套接字API. 
//  ===================================
//
//  我们在下面提供了每个函数调用的原型定义和细节说明, 但这些只是指导性的, 你完全可以根据自己的想法来设计代码.
//
//  注意: 当实现这些函数时, 你需要考虑FSM中所有可能的状态, 这可以使用switch语句来实现. 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

// 这个函数初始化TCB表, 将所有条目标记为NULL. 它还针对重叠网络TCP套接字描述符conn初始化一个STCP层的全局变量, 
// 该变量作为sip_sendseg和sip_recvseg的输入参数. 最后, 这个函数启动seghandler线程来处理进入的STCP段.
// 服务器只有一个seghandler.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
void stcp_server_init(int conn)
{
    int i;
    for (i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) TCB[i] = NULL;
    son_conn = conn;
    
    pthread_t handler;
    pthread_create(&handler,NULL,seghandler,NULL);
    printf("stcp_server_init() success.\n");
    return;
}

// 这个函数查找服务器TCB表以找到第一个NULL条目, 然后使用malloc()为该条目创建一个新的TCB条目.
// 该TCB中的所有字段都被初始化, 例如, TCB state被设置为CLOSED, 服务器端口被设置为函数调用参数server_port. 
// TCB表中条目的索引应作为服务器的新套接字ID被这个函数返回, 它用于标识服务器的连接. 
// 如果TCB表中没有条目可用, 这个函数返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
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
    tcb->server_nodeID = 0;
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

// 这个函数使用sockfd获得TCB指针, 并将连接的state转换为LISTENING. 它然后启动定时器进入忙等待直到TCB状态转换为CONNECTED 
// (当收到SYN时, seghandler会进行状态的转换). 该函数在一个无穷循环中等待TCB的state转换为CONNECTED,  
// 当发生了转换时, 该函数返回1. 你可以使用不同的方法来实现这种阻塞等待.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
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
// 发送数据给STCP服务器
//
// 这个函数发送数据给STCP服务器。
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_server_send(int sockfd, void* data, unsigned int length) {
    if (!sockfdValid(sockfd)) return -1;
    if (TCB[sockfd]->state != CONNECTED) return -1;
    //切片
    char* p = (char*)data;
    int least = (length%MAX_SEG_LEN > 0)?1:0;
    int i,totalseg = length/MAX_SEG_LEN + least;
    pthread_mutex_lock(TCB[sockfd]->sendbufMutex);
    for(i=0; i<totalseg; i++){
        //create seg_t
        seg_t sendsegspace;
        seg_t* sendseg = &sendsegspace;
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
            sip_sendseg(son_conn,&TCB[sockfd]->sendBufunSent->seg);
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

// 接收来自STCP客户端的数据. 请回忆STCP使用的是单向传输, 数据从客户端发送到服务器端.
// 信号/控制信息(如SYN, SYNACK等)则是双向传递. 这个函数每隔RECVBUF_ROLLING_INTERVAL时间
// 就查询接收缓冲区, 直到等待的数据到达, 它然后存储数据并返回1. 如果这个函数失败, 则返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int stcp_server_recv(int sockfd, void* buf, unsigned int length)
{
    //在所有sockfd使用前判断sockfd是否有效。
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

// 这个函数调用free()释放TCB条目. 它将该条目标记为NULL, 成功时(即位于正确的状态)返回1,
// 失败时(即位于错误的状态)返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int stcp_server_close(int sockfd)
{
    //记得free掉所有malloc的空间
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

// 这是由stcp_server_init()启动的线程. 它处理所有来自客户端的进入数据. seghandler被设计为一个调用sip_recvseg()的无穷循环, 
// 如果sip_recvseg()失败, 则说明重叠网络连接已关闭, 线程将终止. 根据STCP段到达时连接所处的状态, 可以采取不同的动作.
// 请查看服务端FSM以了解更多细节.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
void *seghandler(void* arg) {
    seg_t* buffer = (seg_t*)malloc(MAX_SEG_LEN + HDR_LEN);
    while(1){
        char sbuffer[MAX_SEG_LEN + HDR_LEN];
        seg_t* sbuf = (seg_t*)sbuffer;
        int sendrt = sip_recvseg(son_conn,buffer);
        if (sendrt == 1){
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
        printf("recv a %s.\n",ds[tcphdr->type]);
        print_stcphdr(tcphdr);
        if (index == -1){ 
            printf("there is no this port\n");
            continue;
        }
        switch(tcphdr->type){
            case SYN: 
                if (TCB[index]->state == LISTENING || TCB[index]->state == CONNECTED){
                    shdr->src_port = tcphdr->dest_port;
                    shdr->dest_port = tcphdr->src_port;
                    shdr->seq_num = 0;
                    shdr->ack_num = 0;
                    shdr->length = 0;
                    shdr->type = SYNACK;
                    shdr->rcv_win = 0;
                    shdr->checksum = 0;
                    if (sip_sendseg(son_conn,sbuf) > 0){
                        printf("send SYNACK to client:%d success!\n",shdr->dest_port);
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
                    if (sip_sendseg(son_conn,sbuf) > 0){
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
                    if (sip_sendseg(son_conn,sbuf) > 0){
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
                        sip_sendseg(son_conn,&TCB[index]->sendBufunSent->seg);
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
                sip_sendseg(son_conn,&p->seg);
                struct timeval tv;
                gettimeofday(&tv,NULL);
                p->sentTime = (unsigned int)tv.tv_usec;
                p = p->next;
            }
        }
        pthread_mutex_unlock(TCB[sockfd]->sendbufMutex);    
        usleep(SENDBUF_POLLING_INTERVAL/1000);
    }
    return; 
}

// 这个线程持续轮询发送缓冲区以触发超时事件. 如果发送缓冲区非空, 它应一直运行.
// 如果(当前时间 - 第一个已发送但未被确认段的发送时间) > DATA_TIMEOUT, 就发生一次超时事件.
// 当超时事件发生时, 重新发送所有已发送但未被确认段. 当发送缓冲区为空时, 这个线程将终止.
