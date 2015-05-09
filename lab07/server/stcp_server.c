#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
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
    sleep(CLOSEWAIT_TIMEOUT);
    TCB[i]->state = CLOSED;
}
int stcp_server_closewait(int index){
    int* i = malloc(sizeof(int));
    *i = index;
    printf("A client disconnect!\n");
    if (TCB[index]->state != CLOSEWAIT) return -1;
    pthread_t close;
    pthread_create(&close,NULL,closewait,(void*)i);

    return 0;
}
int closeAllstcp(){
    int i;
    for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++)
        if (TCB[i] != NULL && TCB[i]->state!=CLOSED && TCB[i]->state!=CLOSEWAIT){
            TCB[i]->state = CLOSEWAIT;
            stcp_server_closewait(i);
        }
    return 0;
}
/*面向应用层的接口*/

//
//
//  我们在下面提供了每个函数调用的原型定义和细节说明, 但这些只是指导性的, 你完全可以根据自己的想法来设计代码.
//
//  注意: 当实现这些函数时, 你需要考虑FSM中所有可能的状态, 这可以使用switch语句来实现.
//
//  目标: 你的任务就是设计并实现下面的函数原型.
//

// stcp服务器初始化
//
// 这个函数初始化TCB表, 将所有条目标记为NULL. 它还针对重叠网络TCP套接字描述符conn初始化一个STCP层的全局变量,
// 该变量作为sip_sendseg和sip_recvseg的输入参数. 最后, 这个函数启动seghandler线程来处理进入的STCP段.
// 服务器只有一个seghandler.
//

void stcp_server_init(int conn) {
    int i;
    for (i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) TCB[i] = NULL;
    son_conn = conn;
    
    pthread_t handler;
    pthread_create(&handler,NULL,seghandler,NULL);
    printf("stcp_server_init() success.\n");
    return;
}

// 创建服务器套接字
//
// 这个函数查找服务器TCB表以找到第一个NULL条目, 然后使用malloc()为该条目创建一个新的TCB条目.
// 该TCB中的所有字段都被初始化, 例如, TCB state被设置为CLOSED, 服务器端口被设置为函数调用参数server_port. 
// TCB表中条目的索引应作为服务器的新套接字ID被这个函数返回, 它用于标识服务器的连接. 
// 如果TCB表中没有条目可用, 这个函数返回-1.

int stcp_server_sock(unsigned int server_port) {
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
    tcb->recvBuf = NULL;
    tcb->usedBufLen = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    tcb->bufMutex = &mutex;
    return rt;
}

// 接受来自STCP客户端的连接
//
// 这个函数使用sockfd获得TCB指针, 并将连接的state转换为LISTENING. 它然后进入忙等待(busy wait)直到TCB状态转换为CONNECTED 
// (当收到SYN时, seghandler会进行状态的转换). 该函数在一个无穷循环中等待TCB的state转换为CONNECTED,  
// 当发生了转换时, 该函数返回1. 你可以使用不同的方法来实现这种阻塞等待.
//

int stcp_server_accept(int sockfd) {
    server_tcb_t* tcb = TCB[sockfd];
    tcb->state = LISTENING;
    while(tcb->state != CONNECTED){
        usleep(50000);
    }
    printf("A client connect success!\n");
    return 0;
}

// 接收来自STCP客户端的数据
//
// 这个函数接收来自STCP客户端的数据. 你不需要在本实验中实现它.
//
int stcp_server_recv(int sockfd, void* buf, unsigned int length) {
	return 1;
}

// 关闭STCP服务器
//
// 这个函数调用free()释放TCB条目. 它将该条目标记为NULL, 成功时(即位于正确的状态)返回1,
// 失败时(即位于错误的状态)返回-1.
//

int stcp_server_close(int sockfd) {
    int index = sockfd;
    if (TCB[index]->state == CLOSED){ 
        pthread_mutex_t* mutex = TCB[index]->bufMutex;
        pthread_mutex_lock(mutex);
        free(TCB[index]->recvBuf);
        free(TCB[index]);
        TCB[index] = NULL;
        pthread_mutex_unlock(mutex);
        return 1;
    }else{
        return -1;
    }
}

// 处理进入段的线程
//
// 这是由stcp_server_init()启动的线程. 它处理所有来自客户端的进入数据. seghandler被设计为一个调用sip_recvseg()的无穷循环, 
// 如果sip_recvseg()失败, 则说明重叠网络连接已关闭, 线程将终止. 根据STCP段到达时连接所处的状态, 可以采取不同的动作.
// 请查看服务端FSM以了解更多细节.
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
            printf("TCP disconnect!\n");
            closeAllstcp();
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
                        printf("stcp socket %d disconnected. error\n",tcphdr->dest_port);
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
                        printf("send FINACK to client:%d success!\n",shdr->dest_port);
                        TCB[index]->state = CLOSEWAIT;
                        stcp_server_closewait(index);
                    }else{
                        //stcp_server_close(tcphdr->dest_port);
                        printf("stcp socket %d disconnected. error\n",tcphdr->dest_port);
                    }
                }
            case FINACK:
                break;
            case DATA:
                break;
            case DATAACK:
                break;
            default: break;
        }        

    }
    free(buffer);
    printf("seghandler exit.\n");
    return 0;
}
