//ÎÄŒþÃû: client/app_stress_client.c
//
//ÃèÊö: ÕâÊÇÑ¹ÁŠ²âÊÔ°æ±ŸµÄ¿Í»§¶Ë³ÌÐòŽúÂë. ¿Í»§¶ËÊ×ÏÈÍš¹ýÔÚ¿Í»§¶ËºÍ·þÎñÆ÷Ö®ŒäŽŽœšTCPÁ¬œÓ,Æô¶¯ÖØµþÍøÂç²ã.
//È»ºóËüµ÷ÓÃstcp_client_init()³õÊŒ»¯STCP¿Í»§¶Ë. ËüÍš¹ýµ÷ÓÃstcp_client_sock()ºÍstcp_client_connect()ŽŽœšÌ×œÓ×Ö²¢Á¬œÓµœ·þÎñÆ÷.
//È»ºóËü¶ÁÈ¡ÎÄŒþsendthis.txtÖÐµÄÎÄ±ŸÊýŸÝ, œ«ÎÄŒþµÄ³€¶ÈºÍÎÄŒþÊýŸÝ·¢ËÍžø·þÎñÆ÷. 
//Ÿ­¹ýÒ»¶ÎÊ±ºòºó, ¿Í»§¶Ëµ÷ÓÃstcp_client_disconnect()¶Ï¿ªµœ·þÎñÆ÷µÄÁ¬œÓ. 
//×îºó,¿Í»§¶Ëµ÷ÓÃstcp_client_close()¹Ø±ÕÌ×œÓ×Ö. ÖØµþÍøÂç²ãÍš¹ýµ÷ÓÃson_stop()Í£Ö¹.

//ŽŽœšÈÕÆÚ: 2015Äê

//ÊäÈë: ÎÞ

//Êä³ö: STCP¿Í»§¶Ë×ŽÌ¬

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/constants.h"
#include "stcp_client.h"
#include <errno.h>

//ŽŽœšÒ»žöÁ¬œÓ, Ê¹ÓÃ¿Í»§¶Ë¶Ë¿ÚºÅ87ºÍ·þÎñÆ÷¶Ë¿ÚºÅ88. 
#define CLIENTPORT1 87
#define SERVERPORT1 88

//ÔÚ·¢ËÍÎÄŒþºó, µÈŽý5Ãë, È»ºó¹Ø±ÕÁ¬œÓ.
#define WAITTIME 5

//这个函数通过在客户和服务器之间创建TCP连接来启动重叠网络层. 它返回TCP套接字描述符, STCP将使用该描述符发送段. 如果TCP连接失败, 返回-1. 
int son_start() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;

	//menset((char *)&addr,'\0',sizeof(addr));
	addr.sin_family = AF_INET;
	
	inet_pton(AF_INET,SON_IP,&addr.sin_addr);
	addr.sin_port = htons(SON_PORT);

	connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

	return sockfd;
}

//这个函数通过关闭客户和服务器之间的TCP连接来停止重叠网络层
void son_stop(int son_conn) {
	close(son_conn);
}

int main() {
	//ÓÃÓÚ¶ª°üÂÊµÄËæ»úÊýÖÖ×Ó
	srand(time(NULL));

	//Æô¶¯ÖØµþÍøÂç²ã²¢»ñÈ¡ÖØµþÍøÂç²ãTCPÌ×œÓ×ÖÃèÊö·û	
	int son_conn = son_start();
	if(son_conn<0) {
		printf("fail to start overlay network\n");
		exit(1);
	}

	//³õÊŒ»¯stcp¿Í»§¶Ë
	stcp_client_init(son_conn);

	//ÔÚ¶Ë¿Ú87ÉÏŽŽœšSTCP¿Í»§¶ËÌ×œÓ×Ö, ²¢Á¬œÓµœSTCP·þÎñÆ÷¶Ë¿Ú88.
	int sockfd = stcp_client_sock(CLIENTPORT1);
	if(sockfd<0) {
		printf("fail to create stcp client sock");
		exit(1);
	}
	if(stcp_client_connect(sockfd,SERVERPORT1)<0) {
		printf("fail to connect to stcp server\n");
		exit(1);
	}
	printf("client connected to server, client port:%d, server port %d\n",CLIENTPORT1,SERVERPORT1);
	
	//»ñÈ¡sendthis.txtÎÄŒþ³€¶È, ŽŽœš»º³åÇø²¢¶ÁÈ¡ÎÄŒþÖÐµÄÊýŸÝ.
	FILE *f;
	f = fopen("client/sendthis.in","r");
	printf("%d\n",errno);
	assert(f!=NULL);
	fseek(f,0,SEEK_END);
	int fileLen = ftell(f);
	fseek(f,0,SEEK_SET);
	char *buffer = (char*)malloc(fileLen);
	fread(buffer,fileLen,1,f);
	fclose(f);
        
	//Ê×ÏÈ·¢ËÍÎÄŒþ³€¶È, È»ºó·¢ËÍÕûžöÎÄŒþ.
	stcp_client_send(sockfd,&fileLen,sizeof(int));
    stcp_client_send(sockfd, buffer, fileLen);
	free(buffer);

	//µÈŽýÒ»¶ÎÊ±Œä, È»ºó¹Ø±ÕÁ¬œÓ.
	sleep(WAITTIME*5);

	if(stcp_client_disconnect(sockfd)<0) {
		printf("fail to disconnect from stcp server\n");
		exit(1);
	}
	if(stcp_client_close(sockfd)<0) {
		printf("fail to close stcp client\n");
		exit(1);
	}
	//Í£Ö¹ÖØµþÍøÂç²ã
	son_stop(son_conn);
}
