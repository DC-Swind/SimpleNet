//文件名: sip/sip.c
//
//描述: 这个文件实现SIP进程  
//
//创建日期: 2015年

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "../common/seg.h"
#include "../topology/topology.h"
#include "sip.h"
#include "nbrcosttable.h"
#include "dvtable.h"
#include "routingtable.h"

//SIP层等待这段时间让SIP路由协议建立路由路径. 
#define SIP_WAITTIME 20

/**************************************************************/
//声明全局变量
/**************************************************************/
int son_conn; 			//到重叠网络的连接
int stcp_conn;			//到STCP的连接
nbr_cost_entry_t* nct;			//邻居代价表
dv_t* dv0;				//距离矢量表
pthread_mutex_t* dv_mutex;		//距离矢量表互斥量
routingtable_t* routingtable;		//路由表
pthread_mutex_t* routingtable_mutex;	//路由表互斥量

/**************************************************************/
//实现SIP的函数
/**************************************************************/

//SIP进程使用这个函数连接到本地SON进程的端口SON_PORT.
//成功时返回连接描述符, 否则返回-1.
int connectToSON() { 
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
    if (sockfd < 0){
        printf("create socket failed in connectToSON().\n"); 
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, CONNECTION_IP, &addr.sin_addr);
    addr.sin_port = htons(SON_PORT);
    int rt = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (rt<0)
    {
      return -1;
    }
    return sockfd;
}

//这个线程每隔ROUTEUPDATE_INTERVAL时间发送路由更新报文.路由更新报文包含这个节点
//的距离矢量.广播是通过设置SIP报文头中的dest_nodeID为BROADCAST_NODEID,并通过son_sendpkt()发送报文来完成的.
void* routeupdate_daemon(void* arg) {
  while (1)
  {
	sleep(ROUTEUPDATE_INTERVAL);
	int num = topology_getNodeNum();
	sip_pkt_t* package = malloc(sizeof(sip_pkt_t));
	package->header.src_nodeID = topology_getMyNodeID();
	package->header.dest_nodeID = BROADCAST_NODEID;
	package->header.type = 1;
	package->header.length = num*sizeof(dv_entry_t);
	
	pthread_mutex_lock(dv_mutex);

	memcpy(package->data,(char*)dv_entry,num*sizeof(dv_entry_t));
   // int i;
	//for (i=0;i<MAX_ROUTINGTABLE_SLOTS;i++)
	//{
		//routingtable_entry_t* point = routingtable->hash[i];
		//while (point!=NULL)
		//{
			son_sendpkt(BROADCAST_NODEID,package,son_conn);
		//	point = point->next;
		//}
	//}
	pthread_mutex_unlock(dv_mutex);
	
  }
	//你需要编写这里的代码.
  return 0;
}

//这个线程处理来自SON进程的进入报文. 它通过调用son_recvpkt()接收来自SON进程的报文.
//如果报文是SIP报文,并且目的节点就是本节点,就转发报文给STCP进程. 如果目的节点不是本节点,
//就根据路由表转发报文给下一跳.如果报文是路由更新报文,就更新距离矢量表和路由表.
void* pkthandler(void* arg) {
  sip_pkt_t* recvbuffer = malloc(sizeof(sip_pkt_t));
  printf("this is handler!\n");
  while (son_recvpkt(recvbuffer,son_conn)!=-1)
  {
    //sprintf("recv a package!!! type = %d\n",recvbuffer->header.type);
    if (recvbuffer->header.type == 1)
    {
        pthread_mutex_lock(dv_mutex);
        int num = topology_getNodeNum();
        int id = recvbuffer->header.src_nodeID;
        int vc = nbrcosttable_getcost(nct,id);
        dv_entry_t* hh = (dv_entry_t* )recvbuffer->data; 
        int i;
        for (i=0;i<num;i++)
        {
            int j;
            if (hh[i].cost!=INFINITE_COST)
                for (j=0;j<num;j++)
                {
                    if (hh[i].nodeID==dv_entry[j].nodeID&&dv_entry[j].cost>hh[i].cost+vc)
                    {
                        if (dvtable_setcost(dv0,topology_getMyNodeID(),dv_entry[j].nodeID,hh[i].cost+vc)==1)
                            printf("change dv to dest:%d  cost: %d\n",dv_entry[j].nodeID,hh[i].cost+vc);
                        else printf("change dv_table fail!!! \n");
                        routingtable_setnextnode(routingtable,dv_entry[j].nodeID,id);
                    }else;
	      
                }
            else;
        }
      
      /*LinkList* point = lhead;					   //point is local nbr's dvtable
      for (;point!=NULL;point=point->next)                        //each of local nbr
      {
	int vc = nbrcosttable_getcost(nct,point->dv_nbr->nodeID);  //vc is nbr cost
	dv_entry_t* hh = (dv_entry_t* )recvbuffer->data;          //hh is src's dvtable
	int num = topology_getNodeNum();
	int i = 0;
	for (i=0;i<num;i++)					  //each of src's dv
	  if (hh[i].nodeID==point->dv_nbr->nodeID&&hh[i].cost!=INFINITE_COST)
	  {
	    int id = recvbuffer->header.src_nodeID;
	    int j=0;
	    for (j=0;j<num;j++)
	      if (dv_entry[j].nodeID==id && dv_entry[j].cost > hh[i].cost+vc)
	      {
		if (dvtable_setcost(dv0,topology_getMyNodeID(),id,hh[i].cost+vc)==1)
		   printf("change dv_table successful!!! \n");else printf("change dv_table fail!!! \n");;
		routingtable_setnextnode(routingtable,id,point->dv_nbr->nodeID);
	      }
	      else;
	  }else;
      }*/
      pthread_mutex_unlock(dv_mutex);
    }else;
    if (recvbuffer->header.type == 2)
    {
      if (recvbuffer->header.dest_nodeID==topology_getMyNodeID())
      {
	seg_t* temp;
	temp = (seg_t* )recvbuffer->data;
	forwardsegToSTCP(stcp_conn,recvbuffer->header.src_nodeID,temp);
      }
      else
      {
	int id = routingtable_getnextnode(routingtable,recvbuffer->header.dest_nodeID);
	son_sendpkt(id,recvbuffer,son_conn);
      }
    }else;
  }
  sip_stop();
  son_conn=-1;
  pthread_exit(NULL);
	//你需要编写这里的代码.
  return 0;
}

//这个函数终止SIP进程, 当SIP进程收到信号SIGINT时会调用这个函数. 
//它关闭所有连接, 释放所有动态分配的内存.
void sip_stop() {
  close(son_conn);
	//你需要编写这里的代码.
  return;
}

//这个函数打开端口SIP_PORT并等待来自本地STCP进程的TCP连接.
//在连接建立后, 这个函数从STCP进程处持续接收包含段及其目的节点ID的sendseg_arg_t. 
//接收的段被封装进数据报(一个段在一个数据报中), 然后使用son_sendpkt发送该报文到下一跳. 下一跳节点ID提取自路由表.
//当本地STCP进程断开连接时, 这个函数等待下一个STCP进程的连接.
void waitSTCP() {
	//你需要编写这里的代码.
    int sockfd;

    sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sockfd < 0){
        printf("create server socket failed in waitSTCP.\n"); 
        return;
    }else ;
    //stcp_conn = sockfd;
    /*Enable address reuse*/
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    //setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

    struct sockaddr_in conn;
    conn.sin_family = AF_INET;
    conn.sin_port = htons((unsigned short)SIP_PORT);
    inet_pton(AF_INET, CONNECTION_IP, &conn.sin_addr);
    if(bind(sockfd,(struct sockaddr*)&conn,sizeof(conn))<0){
        printf("bind socket failed in waitSTCP.\n");
        return;
    }
    if (listen(sockfd,10) < 0){
        printf("socket listen failed in waitSTCP.\n");
        return;
    }
    signal(SIGPIPE,SIG_IGN);
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int connsocket;
START:
    connsocket = accept(sockfd,(struct sockaddr *)&conn,&addrlen);
    
    if (connsocket < 0){
        printf("accept error in waitSTCP.\n");
    }else{
        stcp_conn = connsocket;
        printf("SIP connected SON in waitSTCP.\n");
    }
    
    seg_t recvbuffer;
    int dest;
    while (getsegToSend(stcp_conn,&dest,&recvbuffer)!=-1)
    {
        printf("recv a pkt from STCP in waitSTCP.\n");
      //sendseg_arg_t* temp = malloc(sizeof(sendseg_arg_t));
      int id =routingtable_getnextnode(routingtable,dest);
      sip_pkt_t temp;
      temp.header.src_nodeID = topology_getMyNodeID();
      temp.header.dest_nodeID = dest;
      temp.header.length = sizeof(seg_t);
      temp.header.type = 2;
      memcpy(temp.data,(char* )&recvbuffer,sizeof(seg_t)); 
        if (son_sendpkt(id,&temp,son_conn) == 1){
            printf("Resend the pkt to SON success.next nodeID is %d\n",id);
        }else{
            printf("Resend the pkt to SON failed.\n");
        }
    }
     goto START;
/*
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
    */
    
    return;
}

int main(int argc, char *argv[]) {
	printf("SIP layer is starting, pls wait...\n");

	//初始化全局变量
	nct = nbrcosttable_create();
	dv0 = dvtable_create();
	dv_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(dv_mutex,NULL);
	routingtable = routingtable_create();
	routingtable_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(routingtable_mutex,NULL);
	son_conn = -1;
	stcp_conn = -1;

	nbrcosttable_print(nct);
	dvtable_print(dv0);
	routingtable_print(routingtable);

	//注册用于终止进程的信号句柄
	signal(SIGINT, sip_stop);

	printf("---------------- print over ----------------------\n");
	//连接到本地SON进程 
	son_conn = connectToSON();
	if(son_conn<0) {
		printf("can't connect to SON process\n");
		exit(1);		
	}else
	{
	  printf("connect SON successful son_conn = %d\n",son_conn);
	}
	
	//启动线程处理来自SON进程的进入报文 
	pthread_t pkt_handler_thread; 
	pthread_create(&pkt_handler_thread,NULL,pkthandler,(void*)0);

	//启动路由更新线程 
	pthread_t routeupdate_thread;
	pthread_create(&routeupdate_thread,NULL,routeupdate_daemon,(void*)0);	

	printf("SIP layer is started...\n");
	printf("waiting for routes to be established\n");
	sleep(SIP_WAITTIME);
	routingtable_print(routingtable);

	//等待来自STCP进程的连接
	printf("waiting for connection from STCP process\n");
	waitSTCP(); 

}


