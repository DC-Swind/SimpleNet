// 文件名: common/pkt.c
// 创建日期: 2015年
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include "pkt.h"

// son_sendpkt()由SIP进程调用, 其作用是要求SON进程将报文发送到重叠网络中. SON进程和SIP进程通过一个本地TCP连接互连.
// 在son_sendpkt()中, 报文及其下一跳的节点ID被封装进数据结构sendpkt_arg_t, 并通过TCP连接发送给SON进程. 
// 参数son_conn是SIP进程和SON进程之间的TCP连接套接字描述符.
// 当通过SIP进程和SON进程之间的TCP连接发送数据结构sendpkt_arg_t时, 使用'!&'和'!#'作为分隔符, 按照'!& sendpkt_arg_t结构 !#'的顺序发送.
// 如果发送成功, 返回1, 否则返回-1.
int son_sendpkt(int nextNodeID, sip_pkt_t* pkt, int son_conn)
{
    sendpkt_arg_t *sendpkt = (sendpkt_arg_t*)malloc(sizeof(sendpkt_arg_t));
    sendpkt->nextNodeID = nextNodeID;
    memcpy((char*)&sendpkt->pkt,(char*)pkt,sizeof(sip_pkt_t));
    

    char start[2]; start[0]='!';start[1] = '&';
    char end[2]; end[0]='!';end[1] = '#';
    if (send(son_conn,start,2,0) < 0){
        return -1;
    }
    //checksum(segPtr);
    //printf("send checksum is %d.\n",segPtr->header.checksum);
    if (send(son_conn,(char*)sendpkt,sizeof(sendpkt_arg_t),0) < 0){
        return -1;
    }
    if (send(son_conn,end,2,0) < 0){
        return -1;
    } 
    return 1;
}

// son_recvpkt()函数由SIP进程调用, 其作用是接收来自SON进程的报文. 
// 参数son_conn是SIP进程和SON进程之间TCP连接的套接字描述符. 报文通过SIP进程和SON进程之间的TCP连接发送, 使用分隔符!&和!#. 
// 为了接收报文, 这个函数使用一个简单的有限状态机FSM
// PKTSTART1 -- 起点 
// PKTSTART2 -- 接收到'!', 期待'&' 
// PKTRECV -- 接收到'&', 开始接收数据
// PKTSTOP1 -- 接收到'!', 期待'#'以结束数据的接收 
// 如果成功接收报文, 返回1, 否则返回-1.
int son_recvpkt(sip_pkt_t* pkt, int son_conn)
{
    //return 2 mean the TCP is disconnected!
    int state = 0,len=-1;
    char c;
    while(state == 0 || state == 1){
        len = recv(son_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("son_recvpkt recv !& failed!%d\n",len);
            return -1;
        }
        if (state == 1)
            if (c == '&') state = 2;
            else if (c == '!') state = 1; else state = 0;
        else;
        if (state == 0){
            if (c == '!') state = 1;
        }else;
    }
    int i;
    len = recv(son_conn,(char*)&pkt->header,sizeof(sip_hdr_t),0);
    if (len < sizeof(sip_hdr_t)){ 
        printf("son_recvpkt recv sip_hdr_t failed!\n");
        return -1;
    }
    
    state = 3;

    int datalen = pkt->header.length;
    for(i=0; i<datalen; i++){
        len = recv(son_conn,&pkt->data[i],sizeof(char),0);
        if (len < 1){
            printf("son_recvpkt recv sip_data failed!\n");
            return -1;
        }
    }
    state = 4;
    while(state == 4 || state == 5){
        len = recv(son_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("son_recvpkt recv !# failed!\n");
            return -1;
        }
        if (state == 5)
            if (c == '#') state = 6;
            else if (c == '!') state = 5; else state = 4;
        else;
        if (state == 4)
            if (c == '!') state = 5;
    }
    if (state==6/*&& seglost(segPtr) == 0 && checkchecksum(segPtr)==1*/) return 1; else{
        printf("son_recvpkt recv seglost or checksum error.\n");
        return -1;
    }
}

// 这个函数由SON进程调用, 其作用是接收数据结构sendpkt_arg_t.
// 报文和下一跳的节点ID被封装进sendpkt_arg_t结构.
// 参数sip_conn是在SIP进程和SON进程之间的TCP连接的套接字描述符. 
// sendpkt_arg_t结构通过SIP进程和SON进程之间的TCP连接发送, 使用分隔符!&和!#. 
// 为了接收报文, 这个函数使用一个简单的有限状态机FSM
// PKTSTART1 -- 起点 
// PKTSTART2 -- 接收到'!', 期待'&' 
// PKTRECV -- 接收到'&', 开始接收数据
// PKTSTOP1 -- 接收到'!', 期待'#'以结束数据的接收
// 如果成功接收sendpkt_arg_t结构, 返回1, 否则返回-1.
int getpktToSend(sip_pkt_t* pkt, int* nextNode,int sip_conn)
{
    int state = 0,len=-1;
    char c;
    while(state == 0 || state == 1){
        len = recv(sip_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("getpktToSend recv !& failed!%d\n",len);
            return -1;
        }
        if (state == 1)
            if (c == '&') state = 2;
            else if (c == '!') state = 1; else state = 0;
        else;
        if (state == 0)
            if (c == '!') state = 1;
    }
    int i;
    len = recv(sip_conn,(char*)nextNode,sizeof(int),0);
    if (len < sizeof(int)){ 
        printf("getpktToSend recv nextNode failed!\n");
        return -1;
    }
    
    len = recv(sip_conn,(char*)&pkt->header,sizeof(sip_hdr_t),0);
    if (len < sizeof(sip_hdr_t)){
        printf("getpktToSend recv sip_hdr_t failed!\n");
        return -1;
    }
    state = 3;

    int datalen = pkt->header.length;
    for(i=0; i<datalen; i++){
        len = recv(sip_conn,&pkt->data[i],sizeof(char),0);
        if (len < 1){
            printf("getpktToSend recv sip_data failed!\n");
            return -1;
        }
    }
    state = 4;
    while(state == 4 || state == 5){
        len = recv(sip_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("getpktToSend recv !# failed!\n");
            return -1;
        }
        if (state == 5)
            if (c == '#') state = 6;
            else if (c == '!') state = 5; else state = 4;
        else;
        if (state == 4)
            if (c == '!') state = 5;
    }
    //printf("sip recv !# success!\n");
    if (state==6/*&& seglost(segPtr) == 0 && checkchecksum(segPtr)==1*/) return 1; else{
        printf("getpktToSend recv seglost or checksum error.\n");
        return -1;
    }
}

// forwardpktToSIP()函数是在SON进程接收到来自重叠网络中其邻居的报文后被调用的. 
// SON进程调用这个函数将报文转发给SIP进程. 
// 参数sip_conn是SIP进程和SON进程之间的TCP连接的套接字描述符. 
// 报文通过SIP进程和SON进程之间的TCP连接发送, 使用分隔符!&和!#, 按照'!& 报文 !#'的顺序发送. 
// 如果报文发送成功, 返回1, 否则返回-1.
int forwardpktToSIP(sip_pkt_t* pkt, int sip_conn)
{
    char start[2]; start[0]='!';start[1] = '&';
    char end[2]; end[0]='!';end[1] = '#';
    if (send(sip_conn,start,2,0) < 0){
        return -1;
    }
    //checksum(segPtr);
    //printf("send checksum is %d.\n",segPtr->header.checksum);
    if (send(sip_conn,(char*)pkt,sizeof(sip_pkt_t),0) < 0){
        return -1;
    }
    if (send(sip_conn,end,2,0) < 0){
        return -1;
    } 
    return 1;
}

// sendpkt()函数由SON进程调用, 其作用是将接收自SIP进程的报文发送给下一跳.
// 参数conn是到下一跳节点的TCP连接的套接字描述符.
// 报文通过SON进程和其邻居节点之间的TCP连接发送, 使用分隔符!&和!#, 按照'!& 报文 !#'的顺序发送. 
// 如果报文发送成功, 返回1, 否则返回-1.
int sendpkt(sip_pkt_t* pkt, int conn)
{
    char start[2]; start[0]='!';start[1] = '&';
    char end[2]; end[0]='!';end[1] = '#';
    if (send(conn,start,2,0) < 0){
        return -1;
    }
    if (send(conn,(char*)pkt,sizeof(sip_pkt_t),0) < 0){
        return -1;
    }
    if (send(conn,end,2,0) < 0){
        return -1;
    } 

    return 1;
}

// recvpkt()函数由SON进程调用, 其作用是接收来自重叠网络中其邻居的报文.
// 参数conn是到其邻居的TCP连接的套接字描述符.
// 报文通过SON进程和其邻居之间的TCP连接发送, 使用分隔符!&和!#. 
// 为了接收报文, 这个函数使用一个简单的有限状态机FSM
// PKTSTART1 -- 起点 
// PKTSTART2 -- 接收到'!', 期待'&' 
// PKTRECV -- 接收到'&', 开始接收数据
// PKTSTOP1 -- 接收到'!', 期待'#'以结束数据的接收 
// 如果成功接收报文, 返回1, 否则返回-1.
int recvpkt(sip_pkt_t* pkt, int conn)
{
    //return 2 mean the TCP is disconnected!
    int state = 0,len=-1;
    char c;
    while(state == 0 || state == 1){
        len = recv(conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("recvpkt recv !& failed!%d\n",len);
            return -1;
        }
        if (state == 1)
            if (c == '&') state = 2;
            else if (c == '!') state = 1; else state = 0;
        else;
        if (state == 0)
            if (c == '!') state = 1; 
    }
    int i;
    len = recv(conn,(char*)&pkt->header,sizeof(sip_hdr_t),0);
    if (len < sizeof(sip_hdr_t)){ 
        printf("recvpkt recv sip_hdr_t failed!\n");
        return -1;
    }
    
    state = 3;

    int datalen = pkt->header.length;
    for(i=0; i<datalen; i++){
        len = recv(conn,&pkt->data[i],sizeof(char),0);
        if (len < 1){
            printf("recvpkt recv sip_data failed!\n");
            return -1;
        }
    }
    state = 4;
    while(state == 4 || state == 5){
        len = recv(conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("recvpkt recv !# failed!\n");
            return -1;
        }
        if (state == 5)
            if (c == '#') state = 6;
            else if (c == '!') state = 5; else state = 4;
        else;
        if (state == 4)
            if (c == '!') state = 5;
    }
    if (state==6/*&& seglost(segPtr) == 0 && checkchecksum(segPtr)==1*/) return 1; else{
        printf("recvpkt recv seglost or checksum error.\n");
        return -1;
    }
}
