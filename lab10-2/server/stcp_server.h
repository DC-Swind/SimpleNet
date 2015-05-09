// �ļ���: stcp_server.h
//
// ����: ����ļ�����������״̬����, һЩ��Ҫ�����ݽṹ�ͷ�����STCP�׽��ֽӿڶ���. ����Ҫʵ��������Щ�ӿ�.
//
// ��������: 2015��
//

#ifndef STCPSERVER_H
#define STCPSERVER_H

#include <pthread.h>
#include "../common/seg.h"
#include "../common/constants.h"

//FSM��ʹ�õķ�����״̬
#define	CLOSED 1
#define	LISTENING 2
#define	CONNECTED 3
#define	CLOSEWAIT 4



//�ڷ��ͻ����������д洢�εĵ�Ԫ.
typedef struct segBuf {
        seg_t seg;
        unsigned int sentTime;
        struct segBuf* next;
} segBuf_t;

//������������ƿ�. һ��STCP���ӵķ�������ʹ��������ݽṹ��¼������Ϣ.
typedef struct server_tcb {
    unsigned int server_nodeID;     //�������ڵ�ID, ����IP��ַ, ��ǰδʹ��
    unsigned int server_portNum;    //�������˿ں�
    unsigned int client_nodeID;     //�ͻ��˽ڵ�ID, ����IP��ַ, ��ǰδʹ��
    unsigned int client_portNum;    //�ͻ��˶˿ں�
    unsigned int state;             //������״̬
    unsigned int expect_seqNum;     //�������ڴ����������
    unsigned int next_seqNum;       //*�¶�׼��ʹ�õ���һ����� 
    
    char* recvBuf;                  //ָ����ջ�������ָ��
    unsigned int  usedBufLen;       //���ջ��������ѽ������ݵĴ�С
    pthread_mutex_t* bufMutex;      //ָ��һ����������ָ��, �û��������ڶԽ��ջ������ķ���

    segBuf_t* sendBufHead;          //���ͻ�����ͷ
    segBuf_t* sendBufunSent;        //���ͻ������еĵ�һ��δ���Ͷ�
    segBuf_t* sendBufTail;          //���ͻ�����β
    unsigned int unAck_segNum;      //�ѷ��͵�δ�յ�ȷ�϶ε�����
    pthread_mutex_t* sendbufMutex;  //���ͻ�����������

} server_tcb_t;

extern server_tcb_t* TCB[MAX_TRANSPORT_CONNECTIONS];

//
//  ���ڷ�������Ӧ�ó����STCP�׽���API. 
//  ===================================
//
//  �����������ṩ��ÿ���������õ�ԭ�Ͷ����ϸ��˵��, ����Щֻ��ָ���Ե�, ����ȫ���Ը����Լ����뷨����ƴ���.
//
//  ע��: ��ʵ����Щ����ʱ, ����Ҫ����FSM�����п��ܵ�״̬, �����ʹ��switch�����ʵ��.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

void stcp_server_init(int conn);

// ���������ʼ��TCB��, ��������Ŀ���ΪNULL. �������TCP�׽���������conn��ʼ��һ��STCP���ȫ�ֱ���, 
// �ñ�����Ϊsip_sendseg��sip_recvseg���������. ���, �����������seghandler�߳������������STCP��.
// ������ֻ��һ��seghandler.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_server_sock(unsigned int server_port);

// ����������ҷ�����TCB�����ҵ���һ��NULL��Ŀ, Ȼ��ʹ��malloc()Ϊ����Ŀ����һ���µ�TCB��Ŀ.
// ��TCB�е������ֶζ�����ʼ��, ����, TCB state������ΪCLOSED, �������˿ڱ�����Ϊ�������ò���server_port. 
// TCB������Ŀ������Ӧ��Ϊ�����������׽���ID�������������, �����ڱ�ʶ�������˵�����. 
// ���TCB����û����Ŀ����, �����������-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_server_accept(int sockfd);

// �������ʹ��sockfd���TCBָ��, �������ӵ�stateת��ΪLISTENING. ��Ȼ��������ʱ������æ�ȴ�ֱ��TCB״̬ת��ΪCONNECTED 
// (���յ�SYNʱ, seghandler�����״̬��ת��). �ú�����һ������ѭ���еȴ�TCB��stateת��ΪCONNECTED,  
// ��������ת��ʱ, �ú�������1. �����ʹ�ò�ͬ�ķ�����ʵ�����������ȴ�.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int stcp_server_send(int sockfd, void* data, unsigned int length);
int stcp_server_recv(int sockfd, void* buf, unsigned int length);

// ��������STCP�ͻ��˵�����. �����STCPʹ�õ��ǵ�����, ���ݴӿͻ��˷��͵���������.
// �ź�/������Ϣ(��SYN, SYNACK��)����˫�򴫵�. �������ÿ��RECVBUF_POLLING_INTERVALʱ��
// �Ͳ�ѯ���ջ�����, ֱ���ȴ������ݵ���, ��Ȼ��洢���ݲ�����1. ����������ʧ��, �򷵻�-1.
//
// ע��: stcp_server_recv�ڷ������ݸ�Ӧ�ó���֮ǰ, �������ȴ��û�������ֽ���(��length)���������.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_server_close(int sockfd);

// �����������free()�ͷ�TCB��Ŀ. ��������Ŀ���ΪNULL, �ɹ�ʱ(��λ����ȷ��״̬)����1,
// ʧ��ʱ(��λ�ڴ����״̬)����-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

void* seghandler(void* arg);

// ������stcp_server_init()�������߳�. �������������Կͻ��˵Ľ�������. seghandler�����Ϊһ������sip_recvseg()������ѭ��, 
// ���sip_recvseg()ʧ��, ��˵����SIP���̵������ѹر�, �߳̽���ֹ. ����STCP�ε���ʱ����������״̬, ���Բ�ȡ��ͬ�Ķ���.
// ��鿴�����FSM���˽����ϸ��.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
void* sendBuf_timer(void* clienttcb);

// ����̳߳�����ѯ���ͻ������Դ�����ʱ�¼�. ������ͻ������ǿ�, ��Ӧһֱ����.
// ���(��ǰʱ�� - ��һ���ѷ��͵�δ��ȷ�϶εķ���ʱ��) > DATA_TIMEOUT, �ͷ���һ�γ�ʱ�¼�.
// ����ʱ�¼�����ʱ, ���·��������ѷ��͵�δ��ȷ�϶�. �����ͻ�����Ϊ��ʱ, ����߳̽���ֹ.


#endif