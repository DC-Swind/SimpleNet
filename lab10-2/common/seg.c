#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include "seg.h"

//STCP进程使用这个函数发送sendseg_arg_t结构(包含段及其目的节点ID)给SIP进程.
//参数sip_conn是在STCP进程和SIP进程之间连接的TCP描述符.
//如果sendseg_arg_t发送成功,就返回1,否则返回-1.
int sip_sendseg(int sip_conn, int dest_nodeID, seg_t* segPtr)
{
    sendseg_arg_t sendpkt;
    sendpkt.nodeID = dest_nodeID;
    memcpy(&sendpkt.seg,segPtr,sizeof(seg_t));
    
    char start[2]; start[0]='!';start[1] = '&';
    char end[2]; end[0]='!';end[1] = '#';
    if (send(sip_conn,start,2,0) < 0){
        return -1;
    }
    checksum(&sendpkt.seg);
    if (send(sip_conn,(char *)&sendpkt,sizeof(sendseg_arg_t),0)<0){
        printf("send error in sip_sendseg().\n");
        return -1;
    }
    
    if (send(sip_conn,end,2,0) < 0){
        return -1;
    } 
    return 1;
}

//STCP进程使用这个函数来接收来自SIP进程的包含段及其源节点ID的sendseg_arg_t结构.
//参数sip_conn是STCP进程和SIP进程之间连接的TCP描述符.
//当接收到段时, 使用seglost()来判断该段是否应被丢弃并检查校验和.
//如果成功接收到sendseg_arg_t就返回1, 否则返回-1.
int sip_recvseg(int sip_conn, int* src_nodeID, seg_t* segPtr)
{
    /*
    sendseg_arg_t recvpkt;
    if (recv(sip_conn,(char*)&recvpkt,sizeof(sendseg_arg_t),0) <= 0){
        printf("recv error in sip_recvseg().\n");
        return -1;
    }
    *src_nodeID = recvpkt.nodeID;
    memcpy(segPtr,(char*)&recvpkt.seg,sizeof(seg_t));
    return 1;
    */
    int state = 0,len=-1;
    char c;
    while(state == 0 || state == 1){
        len = recv(sip_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("sip_recvseg recv !& failed!%d  %d\n",len,errno);
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
    
    len = recv(sip_conn,(char*)src_nodeID,sizeof(int),0);
    if (len < sizeof(int)){ 
        printf("sip_recvseg recv src_nodeID failed!\n");
        return -1;
    }
    
    int i;
    len = recv(sip_conn,(char*)&segPtr->header,sizeof(stcp_hdr_t),0);
    if (len < sizeof(stcp_hdr_t)){ 
        printf("sip_recvseg recv stcp_hdr_t failed!\n");
        return -1;
    }
    
    state = 3;

    int datalen = segPtr->header.length;
    for(i=0; i<datalen; i++){
        len = recv(sip_conn,&segPtr->data[i],sizeof(char),0);
        if (len < 1){
            printf("sip_recvseg recv data failed!\n");
            return -1;
        }
    }
    state = 4;
    while(state == 4 || state == 5){
        len = recv(sip_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("sip_recvseg recv !# failed!\n");
            return -1;
        }
        if (state == 5)
            if (c == '#') state = 6;
            else if (c == '!') state = 5; else state = 4;
        else;
        if (state == 4)
            if (c == '!') state = 5;
    }
    if (state==6){
        if (seglost(segPtr) == 0){
            if (checkchecksum(segPtr)==1) return 1;
            else{
                printf("sip_recvseg recv checksum error.\n");
                return -1;
            }
        }else{
            printf("sip_recvseg recv seglost.\n"); 
            return -1;
        }
    }else{
        printf("sip_recvseg recv error state is not 6.\n");
        return -1;
    }
}

//SIP进程使用这个函数接收来自STCP进程的包含段及其目的节点ID的sendseg_arg_t结构.
//参数stcp_conn是在STCP进程和SIP进程之间连接的TCP描述符.
//如果成功接收到sendseg_arg_t就返回1, 否则返回-1.
int getsegToSend(int stcp_conn, int* dest_nodeID, seg_t* segPtr)
{/*
    sendseg_arg_t recvpkt;
    if (recv(stcp_conn,(char*)&recvpkt,sizeof(sendseg_arg_t),0) <= 0){
        printf("recv error in getsegToSend().\n");
        return -1;
    }
    *dest_nodeID = recvpkt.nodeID;
    memcpy(segPtr,(char*)&recvpkt.seg,sizeof(seg_t));
    return 1;
    */
    int state = 0,len=-1;
    char c;
    while(state == 0 || state == 1){
        len = recv(stcp_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("getsegToSend recv !& failed!%d  %d\n",len,errno);
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
    
    len = recv(stcp_conn,(char*)dest_nodeID,sizeof(int),0);
    if (len < sizeof(int)){ 
        printf("getsegToSend recv dest_nodeID failed!\n");
        return -1;
    }
    
    int i;
    len = recv(stcp_conn,(char*)&segPtr->header,sizeof(stcp_hdr_t),0);
    if (len < sizeof(stcp_hdr_t)){ 
        printf("getsegToSend recv stcp_hdr_t failed!\n");
        return -1;
    }
    
    state = 3;

    int datalen = segPtr->header.length;
    for(i=0; i<datalen; i++){
        len = recv(stcp_conn,&segPtr->data[i],sizeof(char),0);
        if (len < 1){
            printf("getsegToSend recv data failed!\n");
            return -1;
        }
    }
    state = 4;
    while(state == 4 || state == 5){
        len = recv(stcp_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("getsegToSend recv !# failed!\n");
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
        printf("getsegToSend recv seglost or checksum error.\n");
        return -1;
    }
}

//SIP进程使用这个函数发送包含段及其源节点ID的sendseg_arg_t结构给STCP进程.
//参数stcp_conn是STCP进程和SIP进程之间连接的TCP描述符.
//如果sendseg_arg_t被成功发送就返回1, 否则返回-1.
int forwardsegToSTCP(int stcp_conn, int src_nodeID, seg_t* segPtr)
{
    sendseg_arg_t sendpkt;
    sendpkt.nodeID = src_nodeID;
    memcpy((char*)&sendpkt.seg,segPtr,sizeof(seg_t));
    
    char start[2]; start[0]='!';start[1] = '&';
    char end[2]; end[0]='!';end[1] = '#';
    if (send(stcp_conn,start,2,0) < 0){
        return -1;
    }
    
    if (send(stcp_conn,(char*)&sendpkt,sizeof(sendseg_arg_t),0) < 0){
        printf("send error in forwardsegToSTCP().\n");
        return -1;
    }
    
    if (send(stcp_conn,end,2,0) < 0){
        return -1;
    } 
    return 1;
}

// 一个段有PKT_LOST_RATE/2的可能性丢失, 或PKT_LOST_RATE/2的可能性有着错误的校验和.
// 如果数据包丢失了, 就返回1, 否则返回0. 
// 即使段没有丢失, 它也有PKT_LOST_RATE/2的可能性有着错误的校验和.
// 我们在段中反转一个随机比特来创建错误的校验和.
int seglost(seg_t* segPtr)
{
    int random = rand()%100;
    if(random<PKT_LOSS_RATE*100) {
        //50%可能性丢失段
        if(rand()%2==0) {
            //printf("seg lost!!!\n");
            return 1;
        }
        //50%可能性是错误的校验和
        else {
            //获取数据长度
            int len = sizeof(stcp_hdr_t)+segPtr->header.length;
            //获取要反转的随机位
            int errorbit = rand()%(len*8);
            //反转该比特
            char* temp = (char*)segPtr;
            temp = temp + errorbit/8;
            *temp = *temp^(1<<(errorbit%8));
            return 0;
        }
    }
    return 0;
}

//这个函数计算指定段的校验和.
//校验和计算覆盖段首部和段数据. 你应该首先将段首部中的校验和字段清零, 
//如果数据长度为奇数, 添加一个全零的字节来计算校验和.
//校验和计算使用1的补码.
unsigned short checksum(seg_t* segment)
{
    segment->header.checksum = 0;
    unsigned long sum = 0;
    int i,len = sizeof(stcp_hdr_t) + segment->header.length;
    unsigned short *data = (unsigned short*)segment;
    for(i = 0; i < len/2; i++){
        sum += *data;
        data++;
        sum = (sum>>16) + (sum & 0xffff);
    }
    if (len%2 == 1){
        segment->data[segment->header.length] = 0x00;
        sum += *data;
        sum = (sum>>16) + (sum & 0xffff);
    }
    segment->header.checksum = ~sum;
    return ~sum;
}

//这个函数检查段中的校验和, 正确时返回1, 错误时返回-1.
int checkchecksum(seg_t* segment)
{
    unsigned short bfsum = segment->header.checksum;
    segment->header.checksum = 0;
    unsigned short afsum = checksum(segment);
    segment->header.checksum = bfsum;
    //printf("bfsum is %d, afsum is %d.\n",bfsum,afsum);
    if (bfsum == afsum) return 1;
    return -1;
}

int print_stcphdr(stcp_hdr_t *hdr){
    printf("+---------------+---------------+---------------+---------------+\n");
    printf("|src   :%d\t\t        |dst   :%d\t\t        |\n",hdr->src_port,hdr->dest_port);
    printf("|seqnum:%d\t\t        |acknum:%d\t\t        |\n",hdr->seq_num,hdr->ack_num);
    printf("|len   :%d\t|type  :%d\t|win   :%d\t|cksum :%d\t|\n",hdr->length,hdr->type,hdr->rcv_win,hdr->checksum);
    printf("+---------------+---------------+---------------+---------------+\n");
    return 0;
}
