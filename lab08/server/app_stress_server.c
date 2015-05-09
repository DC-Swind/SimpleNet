//�ļ���: server/app_stress_server.c

//����: ����ѹ�����԰汾�ķ������������. ����������ͨ���ڿͻ��˺ͷ�����֮�䴴��TCP����,�����ص������. 
//Ȼ��������stcp_server_init()��ʼ��STCP������. ��ͨ������stcp_server_sock()��stcp_server_accept()����һ���׽��ֲ��ȴ����Կͻ��˵�����.
//��Ȼ������ļ�����. ����֮��, ������һ��������, �����ļ����ݲ��������浽receivedtext.txt�ļ���. 
//���, ������ͨ������stcp_server_close()�ر��׽���. �ص������ͨ������son_stop()ֹͣ.

//��������: 2015��

//����: ��

//���: STCP������״̬

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include "../common/constants.h"
#include "stcp_server.h"

//����һ������, ʹ�ÿͻ��˶˿ں�87�ͷ������˿ں�88. 
#define CLIENTPORT1 87
#define SERVERPORT1 88
//�ڽ��յ��ļ����ݱ������, �������ȴ�10��, Ȼ��ر�����.
#define WAITTIME 10

//�������ͨ���ڿͻ��ͷ�����֮�䴴��TCP�����������ص������. ������TCP�׽���������, STCP��ʹ�ø����������Ͷ�. ���TCP����ʧ��, ����-1.
int son_start() {
    int sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sockfd < 0){
        printf("create server socket failed in son_start().\n"); 
        return -1;
    }else ;//printf("create server socket success in son_start().\n");
    /*Enable address reuse*/
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in conn;
    conn.sin_family = AF_INET;
    conn.sin_port = htons((unsigned short)SON_PORT);
    inet_pton(AF_INET, SON_IP, &conn.sin_addr);
    if(bind(sockfd,(struct sockaddr*)&conn,sizeof(conn))<0){
        printf("bind socket failed in son_start().\n");
        return -1;
    }
    if (listen(sockfd,10) < 0){
        printf("socket listen failed in son_start().\n");
        return 0;
    }
    signal(SIGPIPE,SIG_IGN);

    socklen_t addrlen = sizeof(struct sockaddr_in);
    int connsocket = accept(sockfd,(struct sockaddr *)&conn,&addrlen);
    if (connsocket < 0){
        printf("accept error in son_start().\n");
    }
    printf("son_start() success.\n");
    return connsocket;
}


//�������ͨ���رտͻ��ͷ�����֮���TCP������ֹͣ�ص������
void son_stop(int son_conn) {
    close(son_conn);
}



int main() {
	//���ڶ����ʵ����������
	srand(time(NULL));

	//�����ص�����㲢��ȡ�ص������TCP�׽���������
	int son_conn = son_start();
	if(son_conn<0) {
		printf("can not start overlay network\n");
	}

	//��ʼ��STCP������
	stcp_server_init(son_conn);

	//�ڶ˿�SERVERPORT1�ϴ���STCP�������׽��� 
	int sockfd= stcp_server_sock(SERVERPORT1);
	if(sockfd<0) {
		printf("can't create stcp server\n");
		exit(1);
	}
	//��������������STCP�ͻ��˵����� 
	stcp_server_accept(sockfd);

	//���Ƚ����ļ�����, Ȼ������ļ����� 
	int fileLen;
	stcp_server_recv(sockfd,&fileLen,sizeof(int));
  
	char* buf = (char*) malloc(fileLen);
	stcp_server_recv(sockfd,buf,fileLen);
        printf("@@@@@@@@@start write file!\n");
	//�����յ����ļ����ݱ��浽�ļ�receivedtext.txt��
	FILE* f;
	f = fopen("receivedtext.txt","a");
	fwrite(buf,fileLen,1,f);
	fclose(f);
	free(buf);

	sleep(WAITTIME*5);

	//�ر�STCP������ 
	if(stcp_server_close(sockfd)<0) {
		printf("can't destroy stcp server\n");
		exit(1);
	}				
	//ֹͣ�ص������
	son_stop(son_conn);
}
