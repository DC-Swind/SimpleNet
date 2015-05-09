#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include"logic.h"
#include"commondef.h"
#include"screen.h"
#include"Key_Event.h"
#include<pthread.h>
#include"receive.h"
#include<arpa/inet.h>
#include"stcp_client.h"
#include"../topology/topology.h"
#include<unistd.h>


#define CLIENTPORT1 88

//在连接到SIP进程后, 等待1秒, 让服务器启动.
#define STARTDELAY 1
//在发送字符串后, 等待5秒, 然后关闭连接.
#define WAITTIME 5

//这个函数连接到本地SIP进程的端口SIP_PORT. 如果TCP连接失败, 返回-1. 连接成功, 返回TCP套接字描述符, STCP将使用该描述符发送段.
int connectToSIP() {
	//你需要编写这里的代码.
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0){
        printf("create socket failed in connectToSIP().\n"); 
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, CONNECTION_IP, &addr.sin_addr);
    addr.sin_port = htons(SIP_PORT);
    int rt = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (rt < 0) return -1;
    return sockfd;
}

//这个函数断开到本地SIP进程的TCP连接. 
void disconnectToSIP(int sip_conn) {
	//你需要编写这里的代码.
    close(sip_conn);
}

int main()
{
	//连接到SIP进程并获得TCP套接字描述符	
	int sip_conn = connectToSIP();
	if(sip_conn<0) {
		printf("fail to connect to the local SIP process\n");
		exit(1);
	}

	//初始化stcp客户端
	stcp_client_init(sip_conn);
	sleep(STARTDELAY);

	char hostname[50];
	printf("Enter server name to connect:");
	scanf("%s",hostname);
	int server_nodeID = topology_getNodeIDfromname(hostname);
    printf("server node is :%d \n",server_nodeID);
	if(server_nodeID == -1) {
		printf("host name error!\n");
		exit(1);
	}

	//在端口87上创建STCP客户端套接字, 并连接到STCP服务器端口88
	sockfd = stcp_client_sock(CLIENTPORT1);
	if(sockfd<0) {
		printf("fail to create stcp client sock");
		exit(1);
	}
	if(stcp_client_connect(sockfd,server_nodeID,topology_getMyNodeID())<0) {
		printf("fail to connect to stcp server\n");
		exit(1);
	}
	printf("client connected to server, client port:%d, server port %d\n",CLIENTPORT1,topology_getMyNodeID());
	
	/*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;

	menset((char *)&addr,'\0',sizeof(addr));
	addr.sin_family = AF_INET;
	
	inet_pton(AF_INET,address,&addr.sin_addr);
	addr.sin_port = htons(PORT);

	connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));*/
	
	init_screen();
	interface = 0;
	game_status = 0;
	menset(user_name,'\0',sizeof(user_name));	

	
	int t = 0;
	pthread_t threads[1];
	int rc = pthread_create(&threads[t], NULL, receive, (void *)&t);
	if (rc){
		printf("CREATE THREAD ERROR!\n");
		exit(-1);
	}
		

	
	logic();

	return 0;
}
