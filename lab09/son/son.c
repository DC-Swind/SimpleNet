//文件名: son/son.c
//
//描述: 这个文件实现SON进程 
//SON进程首先连接到所有邻居, 然后启动listen_to_neighbor线程, 每个该线程持续接收来自一个邻居的进入报文, 并将该报文转发给SIP进程. 
//然后SON进程等待来自SIP进程的连接. 在与SIP进程建立连接之后, SON进程持续接收来自SIP进程的sendpkt_arg_t结构, 并将接收到的报文发送到重叠网络中. 
//
//创建日期: 2015年

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/utsname.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "son.h"
#include "../topology/topology.h"
#include "neighbortable.h"

//你应该在这个时间段内启动所有重叠网络节点上的SON进程
#define SON_START_DELAY 60

/**************************************************************/
//声明全局变量
/**************************************************************/

//将邻居表声明为一个全局变量 
nbr_entry_t* nt; 
//将与SIP进程之间的TCP连接声明为一个全局变量
int sip_conn; 

/**************************************************************/
//实现重叠网络函数
/**************************************************************/

// 这个线程打开TCP端口CONNECTION_PORT, 等待节点ID比自己大的所有邻居的进入连接,
// 在所有进入连接都建立后, 这个线程终止. 
void* waitNbrs(void* arg) {
    //你需要编写这里的代码.
    int i,nt_big = 0,mynode = topology_getMyNodeID();
    for(i=0; i< MAX_NODE_NUM; i++) if (nt[i].nodeID > mynode){
		nt_big++;
	}
	
    int sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sockfd < 0){
        printf("create server socket failed in waitNbrs.\n"); 
        return NULL;
    }else ;
    /*Enable address reuse*/
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in conn;
    conn.sin_family = AF_INET;
    conn.sin_port = htons((unsigned short)CONNECTION_PORT);
	
	struct ifreq ifr;
	char *localip;
	strcpy(ifr.ifr_name,"eth0");
    if(ioctl(sockfd,SIOCGIFADDR,&ifr) < 0){
		printf("get local ip failed in eth0, test eth1\n");
		strcpy(ifr.ifr_name,"eth1");
		if(ioctl(sockfd,SIOCGIFADDR,&ifr) < 0){
			printf("get local ip failed in waitNbrs.\n");
			return NULL;
		}else{
			localip = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
		}
    }else{
        localip = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
    }
    
    
	printf("local ip is %s\n",localip);

	inet_pton(AF_INET, localip, &conn.sin_addr);
    if(bind(sockfd,(struct sockaddr*)&conn,sizeof(conn))<0){
        printf("bind socket failed in waitNbrs.\n");
        return NULL;
    }
    if (listen(sockfd,10) < 0){
        printf("socket listen failed in waitNbrs.\n");
        return NULL;
    }
    signal(SIGPIPE,SIG_IGN);

    socklen_t addrlen = sizeof(struct sockaddr_in);
    while(nt_big > 0){
        int connsocket = accept(sockfd,(struct sockaddr *)&conn,&addrlen);
        if (connsocket < 0){
            printf("accept error in waitNbrs.\n");
        }else{
            printf("%s connected in waitNbrs.\n",inet_ntoa(conn.sin_addr));
            for(i=0; i<MAX_NODE_NUM; i++) if (nt[i].nodeIP == conn.sin_addr.s_addr){
                nt[i].conn = connsocket;
                nt_big--;
            }
        }
    }
    printf("end waitNbrs.\n");
    return NULL;
}

// 这个函数连接到节点ID比自己小的所有邻居.
// 在所有外出连接都建立后, 返回1, 否则返回-1.
int connectNbrs() {
    //你需要编写这里的代码.
    int nt_small=0,i,mynode= topology_getMyNodeID();
    for(i=0; i<MAX_NODE_NUM; i++) if (nt[i].nodeID < mynode){
        nt_small++;
    }
   

    for(i=0; i<MAX_NODE_NUM; i++) if(nt[i].nodeID!=-1 && nt[i].nodeID < mynode){
		int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	    if (sockfd < 0){
		    printf("create server socket failed in connectNbrs.\n"); 
            return -1;
	    }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = nt[i].nodeIP;
        addr.sin_port = htons(CONNECTION_PORT);
        int rt = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
        if (rt >=0 ) nt[i].conn = sockfd;
        else{
            printf("connect to nodeID: %d failed in connectNbrs.\n",nt[i].nodeID);
			printf("to nodeID's ip is %s\n",inet_ntoa(addr.sin_addr));
            sleep(1);
            i--;
        }
    }
 
    return 1;
}

//每个listen_to_neighbor线程持续接收来自一个邻居的报文. 它将接收到的报文转发给SIP进程.
//所有的listen_to_neighbor线程都是在到邻居的TCP连接全部建立之后启动的. 
void* listen_to_neighbor(void* arg) {
    //你需要编写这里的代码.
    int index = *(int*)arg;
    int conn = nt[index].conn;
	int recv_failed = 0;
    while(1){
        sip_pkt_t pkt;
        if (recvpkt(&pkt, conn) < 0){
            printf("node index:%d recvpkt failed in listen_to_neighbor.\n",index);
			printf("the conn is %d\n",conn);
			//neighbor failed.
			recv_failed++;
			if (recv_failed > 3){
				nt[index].conn = -1;
				nt[index].nodeID = -1;
				pthread_exit(NULL);
			}
        }else{
            if (forwardpktToSIP(&pkt, sip_conn) < 0){
                printf("node index:%d forwardpktToSIP failed in listen_to_neighbor.\n",index);
            }
        }
    }
 
    return NULL;
}

//这个函数打开TCP端口SON_PORT, 等待来自本地SIP进程的进入连接. 
//在本地SIP进程连接之后, 这个函数持续接收来自SIP进程的sendpkt_arg_t结构, 并将报文发送到重叠网络中的下一跳. 
//如果下一跳的节点ID为BROADCAST_NODEID, 报文应发送到所有邻居节点.
void waitSIP() {
    //你需要编写这里的代码.
	int sockfd;

    sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sockfd < 0){
        printf("create server socket failed in waitSIP.\n"); 
        return;
    }else ;
    /*Enable address reuse*/
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	//setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

    struct sockaddr_in conn;
    conn.sin_family = AF_INET;
    conn.sin_port = htons((unsigned short)SON_PORT);
    inet_pton(AF_INET, "127.0.0.1", &conn.sin_addr);
    if(bind(sockfd,(struct sockaddr*)&conn,sizeof(conn))<0){
        printf("bind socket failed in waitSIP.\n");
        return;
    }
    if (listen(sockfd,10) < 0){
        printf("socket listen failed in waitSIP.\n");
        return;
    }
    signal(SIGPIPE,SIG_IGN);
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int connsocket;
START:
    connsocket = accept(sockfd,(struct sockaddr *)&conn,&addrlen);
    if (connsocket < 0){
        printf("accept error in waitSIP.\n");
    }else{
		sip_conn = connsocket;
        printf("SIP connected SON in waitSIP.\n");
    }

    while(1){
        sip_pkt_t pkt;
        int nextNode;
        if(getpktToSend(&pkt, &nextNode,sip_conn) < 0){
            printf("getpktToSend from SIP failed in waitSIP.\n");
            printf("rewait sip connected!\n");
            //printf("sleep 5 minutes, then rewait SIP to connect. start sleep.\n");
            close(sip_conn);
            //sleep(300);
	    //printf("sleep end,start rewait SIP to connect.\n");		
	    goto START;
        }else{
			printf("get a pkt from sip in waitSIP.\n");
            if (nextNode == BROADCAST_NODEID){
                int j;
                for(j=0; j<MAX_NODE_NUM; j++) if (nt[j].conn > -1){
                    if (sendpkt(&pkt,nt[j].conn) < 0){
                        printf("sendpkt to %d broadcast failed.\n",nt[j].nodeID);
                    }else{
						printf("sendpkt to %d broadcast success.\n",nt[j].nodeID);
					}
                }
            }else{
                if (sendpkt(&pkt,nt[nextNode].conn) < 0){
                    printf("sendpkt to nextNode failed.\n");
                }else; 
            }
        }
    }
    return;

}

//这个函数停止重叠网络, 当接收到信号SIGINT时, 该函数被调用.
//它关闭所有的连接, 释放所有动态分配的内存.
void son_stop() {
    //你需要编写这里的代码.
    int i;
    for(i=0; i<MAX_NODE_NUM;i++) if (nt[i].conn > 0) close(nt[i].conn);
    //free  内存
}

int main() {
	//启动重叠网络初始化工作
	printf("Overlay network: Node %d initializing...\n",topology_getMyNodeID());	
	
	//topology_getMyNodeID();

	//创建一个邻居表
	nt = nt_create();
	//将sip_conn初始化为-1, 即还未与SIP进程连接
	sip_conn = -1;
	
	//注册一个信号句柄, 用于终止进程
	signal(SIGINT, son_stop);

	//打印所有邻居
	int nbrNum = topology_getNbrNum();
	int i;
	for(i=0;i<nbrNum;i++) {
		printf("Overlay network: neighbor %d:%d\n",i+1,nt[i].nodeID);
	}

	//启动waitNbrs线程, 等待节点ID比自己大的所有邻居的进入连接
	pthread_t waitNbrs_thread;
	pthread_create(&waitNbrs_thread,NULL,waitNbrs,(void*)0);

	//等待其他节点启动
	sleep(SON_START_DELAY);
	
	//连接到节点ID比自己小的所有邻居
	connectNbrs();

	//等待waitNbrs线程返回
	pthread_join(waitNbrs_thread,NULL);	

	//此时, 所有与邻居之间的连接都建立好了
	
	//创建线程监听所有邻居
	for(i=0;i<nbrNum;i++) {
		int* idx = (int*)malloc(sizeof(int));
		*idx = i;
		pthread_t nbr_listen_thread;
		pthread_create(&nbr_listen_thread,NULL,listen_to_neighbor,(void*)idx);
	}
	printf("Overlay network: node initialized...\n");
	printf("Overlay network: waiting for connection from SIP process...\n");

	//等待来自SIP进程的连接
	waitSIP();
}
