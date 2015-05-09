// �ļ��� pkt.c
// ��������: 2015��
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include "pkt.h"
#include <errno.h>

// son_sendpkt()��SIP���̵���, ��������Ҫ��SON���̽����ķ��͵��ص�������. SON���̺�SIP����ͨ��һ������TCP���ӻ���.
// ��son_sendpkt()��, ���ļ�����һ���Ľڵ�ID����װ�����ݽṹsendpkt_arg_t, ��ͨ��TCP���ӷ��͸�SON����. 
// ����son_conn��SIP���̺�SON����֮���TCP�����׽���������.
// ��ͨ��SIP���̺�SON����֮���TCP���ӷ������ݽṹsendpkt_arg_tʱ, ʹ��'!&'��'!#'��Ϊ�ָ���, ����'!& sendpkt_arg_t�ṹ !#'��˳����.
// ������ͳɹ�, ����1, ���򷵻�-1.
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

// son_recvpkt()������SIP���̵���, �������ǽ�������SON���̵ı���. 
// ����son_conn��SIP���̺�SON����֮��TCP���ӵ��׽���������. ����ͨ��SIP���̺�SON����֮���TCP���ӷ���, ʹ�÷ָ���!&��!#. 
// Ϊ�˽��ձ���, �������ʹ��һ���򵥵�����״̬��FSM
// PKTSTART1 -- ��� 
// PKTSTART2 -- ���յ�'!', �ڴ�'&' 
// PKTRECV -- ���յ�'&', ��ʼ��������
// PKTSTOP1 -- ���յ�'!', �ڴ�'#'�Խ������ݵĽ��� 
// ����ɹ����ձ���, ����1, ���򷵻�-1.
int son_recvpkt(sip_pkt_t* pkt, int son_conn)
{
    //return 2 mean the TCP is disconnected!
    int state = 0,len=-1;
    char c;
    while(state == 0 || state == 1){
        len = recv(son_conn,&c,sizeof(c),0);
        if (len <= 0){
            printf("son_recvpkt recv !& failed!%d  %d\n",len,errno);
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

// ���������SON���̵���, �������ǽ������ݽṹsendpkt_arg_t.
// ���ĺ���һ���Ľڵ�ID����װ��sendpkt_arg_t�ṹ.
// ����sip_conn����SIP���̺�SON����֮���TCP���ӵ��׽���������. 
// sendpkt_arg_t�ṹͨ��SIP���̺�SON����֮���TCP���ӷ���, ʹ�÷ָ���!&��!#. 
// Ϊ�˽��ձ���, �������ʹ��һ���򵥵�����״̬��FSM
// PKTSTART1 -- ��� 
// PKTSTART2 -- ���յ�'!', �ڴ�'&' 
// PKTRECV -- ���յ�'&', ��ʼ��������
// PKTSTOP1 -- ���յ�'!', �ڴ�'#'�Խ������ݵĽ���
// ����ɹ�����sendpkt_arg_t�ṹ, ����1, ���򷵻�-1.
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

// forwardpktToSIP()��������SON���̽��յ������ص����������ھӵı��ĺ󱻵��õ�. 
// SON���̵����������������ת����SIP����. 
// ����sip_conn��SIP���̺�SON����֮���TCP���ӵ��׽���������. 
// ����ͨ��SIP���̺�SON����֮���TCP���ӷ���, ʹ�÷ָ���!&��!#, ����'!& ���� !#'��˳����. 
// ������ķ��ͳɹ�, ����1, ���򷵻�-1.
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

// sendpkt()������SON���̵���, �������ǽ�������SIP���̵ı��ķ��͸���һ��.
// ����conn�ǵ���һ���ڵ��TCP���ӵ��׽���������.
// ����ͨ��SON���̺����ھӽڵ�֮���TCP���ӷ���, ʹ�÷ָ���!&��!#, ����'!& ���� !#'��˳����. 
// ������ķ��ͳɹ�, ����1, ���򷵻�-1.
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

// recvpkt()������SON���̵���, �������ǽ��������ص����������ھӵı���.
// ����conn�ǵ����ھӵ�TCP���ӵ��׽���������.
// ����ͨ��SON���̺����ھ�֮���TCP���ӷ���, ʹ�÷ָ���!&��!#. 
// Ϊ�˽��ձ���, �������ʹ��һ���򵥵�����״̬��FSM
// PKTSTART1 -- ��� 
// PKTSTART2 -- ���յ�'!', �ڴ�'&' 
// PKTRECV -- ���յ�'&', ��ʼ��������
// PKTSTOP1 -- ���յ�'!', �ڴ�'#'�Խ������ݵĽ��� 
// ����ɹ����ձ���, ����1, ���򷵻�-1.
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
