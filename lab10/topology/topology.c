//文件名: topology/topology.c
//
//描述: 这个文件实现一些用于解析拓扑文件的辅助函数 
//
//创建日期: 2015年
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include "../common/constants.h"
#include "topology.h"

//这个函数返回指定主机的节点ID.
//节点ID是节点IP地址最后8位表示的整数.
//例如, 一个节点的IP地址为202.119.32.12, 它的节点ID就是12.
//如果不能获取节点ID, 返回-1.
int topology_getNodeIDfromname(char* hostname) 
{
    /*
	int len = strlen(hostname);
    int i=0,n=0;
    while(i < len){
		if (hostname[i] == '.') n++;
		i++;
		if (n >= 3) break;
    }
    if (len-1 == i) return hostname[i]-'0';
    if (len-2 == i) return (hostname[i]-'0')*10 + (hostname[i+1]-'0');
    if (len-3 == i) return (hostname[i]-'0')*100 + (hostname[i+1]-'0')*10+(hostname[i+2]-'0');
    return -1;
	*/
	int len = strlen(hostname);
    int i=0,n=0;
    while(i < len){
		if (hostname[i] == '.') n++;
		i++;
		if (n >= 3) break;
    }
    if (len-1 == i) return hostname[i]-'0';
    if (len-2 == i) return (hostname[i]-'0')*10 + (hostname[i+1]-'0');
    if (len-3 == i) return (hostname[i]-'0')*100 + (hostname[i+1]-'0')*10+(hostname[i+2]-'0');
    return 0;
}

//这个函数返回指定的IP地址的节点ID.
//如果不能获取节点ID, 返回-1.
int topology_getNodeIDfromip(struct in_addr* addr)
{
    char *hostname = inet_ntoa(*addr);
    int len = strlen(hostname);
    int i=0,n=0;
    while(i < len){
		if (hostname[i] == '.') n++;
		i++;
		if (n >= 3) break;
    }
    if (len-1 == i) return hostname[i]-'0';
    if (len-2 == i) return (hostname[i]-'0')*10 + (hostname[i+1]-'0');
    if (len-3 == i) return (hostname[i]-'0')*100 + (hostname[i+1]-'0')*10+(hostname[i+2]-'0');
    return 0;
}

//这个函数返回本机的节点ID
//如果不能获取本机的节点ID, 返回-1.
int topology_getMyNodeID()
{
    /*
    struct hostent *he;
    char hostname[20] = {0};
    gethostname(hostname,sizeof(hostname));
    he = gethostbyname(hostname);
    printf("hostname is %s in topology.\n",hostname);
    printf("%s\n",inet_ntoa(*(struct in_addr*)(he->h_addr)));
    return 0;
    */
    struct ifreq ifr;
    int inet_sock;
    if ((inet_sock = socket(AF_INET,SOCK_DGRAM,0)) < 0){
        perror("socket error in topology get my node id.");
    }
    //strcpy(ifr.ifr_name,"wlan0");
    strcpy(ifr.ifr_name,"eth0");
    if(ioctl(inet_sock,SIOCGIFADDR,&ifr) < 0){
		strcpy(ifr.ifr_name,"eth1");
		if(ioctl(inet_sock,SIOCGIFADDR,&ifr) < 0){
			printf("no eth1 in topology_getMyNodeID.\n");
            close(inet_sock);
			return -1;
		}else{
			struct in_addr* addr = &(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
            close(inet_sock);
			return topology_getNodeIDfromip(addr);
		}
    }else{
        struct in_addr* addr = &(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
        //char* p = inet_ntoa(*addr);
        //printf("local ip addr is %s.\n",p);
        close(inet_sock);
        return topology_getNodeIDfromip(addr);
    }
    close(inet_sock);
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回邻居数.
int topology_getNbrNum()
{
	FILE *fp = fopen("topology/topology.dat", "r");
    int i,f[256],sum=0;
    for(i = 0; i< 256; i++) f[i] = 1;
    int mynode = topology_getMyNodeID();
    while (!feof(fp))
    {
        char ip1[20],ip2[20];
        int len;
        fscanf(fp, "%s", ip1);
        fscanf(fp, "%s", ip2);
        fscanf(fp, "%d", &len);
        int node1 = topology_getNodeIDfromname(ip1);
        int node2 = topology_getNodeIDfromname(ip2);
        if (node1 == mynode && f[node2]){
            f[node2] = 0;
            sum++;
        }
        if (node2 == mynode && f[node1]){
            f[node1] = 0;
            sum++;
        }
    }
    fclose(fp);
    return sum;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回重叠网络中的总节点数.
int topology_getNodeNum()
{
    FILE *fp = fopen("topology/topology.dat", "r");
    int i,f[256],sum=0;
    for(i = 0; i< 256; i++) f[i] = 1;
    while (!feof(fp))
    {
        char ip1[20],ip2[20];
        int len;
        fscanf(fp, "%s", ip1);
        fscanf(fp, "%s", ip2);
        fscanf(fp, "%d", &len);
        int node1 = topology_getNodeIDfromname(ip1);
        int node2 = topology_getNodeIDfromname(ip2);
        if (f[node1]){
            f[node1] = 0;
            sum++;
        }
        if (f[node2]){
            f[node2] = 0;
            sum++;
        }
    }
    fclose(fp);
    return sum;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回一个动态分配的数组, 它包含重叠网络中所有节点的ID. 
int* topology_getNodeArray()
{
    FILE *fp = fopen("topology/topology.dat", "r");
    int i,f[256],sum=0;
    int rttmp[20];
    for(i = 0; i< 256; i++) f[i] = 1;
    while (!feof(fp))
    {
        char ip1[20],ip2[20];
        int len;
        fscanf(fp, "%s", ip1);
        fscanf(fp, "%s", ip2);
        fscanf(fp, "%d", &len);
        int node1 = topology_getNodeIDfromname(ip1);
        int node2 = topology_getNodeIDfromname(ip2);
        if (f[node1]){
            f[node1] = 0;
            rttmp[sum] = node1;
            sum++;
        }
        if (f[node2]){
            f[node2] = 0;
            rttmp[sum] = node2;
            sum++;
        }
    }
    fclose(fp);
    if (sum == 0) return NULL;
    int *rt = (int*)malloc(sum * sizeof(int));
    for(i=0; i<sum; i++) rt[i] = rttmp[i];
    return rt;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回一个动态分配的数组, 它包含所有邻居的节点ID.  
int* topology_getNbrArray()
{
    FILE *fp = fopen("topology/topology.dat", "r");
    int i,f[256],sum=0,rttmp[20];
    for(i = 0; i< 256; i++) f[i] = 1;
    int mynode = topology_getMyNodeID();
    while (!feof(fp))
    {
        char ip1[20],ip2[20];
        int len;
        fscanf(fp, "%s", ip1);
        fscanf(fp, "%s", ip2);
        fscanf(fp, "%d", &len);
        int node1 = topology_getNodeIDfromname(ip1);
        int node2 = topology_getNodeIDfromname(ip2);
        if (node1 != mynode && f[node1]){
            f[node1] = 0;
            rttmp[sum] = node1;
            sum++;
        }
        if (node2 != mynode && f[node2]){
            f[node2] = 0;
            rttmp[sum] = node2;
            sum++;
        }
    }
    fclose(fp);
    if (sum == 0) return NULL;
    int *rt = (int *)malloc(sum * sizeof(int));
    for(i = 0; i<sum ;i++) rt[i] = rttmp[i];
    return rt;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回指定两个节点之间的直接链路代价. 
//如果指定两个节点之间没有直接链路, 返回INFINITE_COST.
unsigned int topology_getCost(int fromNodeID, int toNodeID)
{
    FILE *fp = fopen("topology/topology.dat", "r");
    while (!feof(fp))
    {
        char ip1[20],ip2[20];
        int len;
        fscanf(fp, "%s", ip1);
        fscanf(fp, "%s", ip2);
        fscanf(fp, "%d", &len);
        int node1 = topology_getNodeIDfromname(ip1);
        int node2 = topology_getNodeIDfromname(ip2);
        if (node1 == fromNodeID && node2 == toNodeID){
            fclose(fp);
            return len;
        }else if(node2 == fromNodeID && node1 == toNodeID){
            fclose(fp);
            return len;
        }
    }
    fclose(fp);
    return INFINITE_COST;
}
