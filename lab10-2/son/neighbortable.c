//文件名: son/neighbortable.c
//
//描述: 这个文件实现用于邻居表的API
//
//创建日期: 2015年
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "neighbortable.h"
#include "../common/constants.h"
#include "../topology/topology.h"

//这个函数首先动态创建一个邻居表. 然后解析文件topology/topology.dat, 填充所有条目中的nodeID和nodeIP字段, 将conn字段初始化为-1.
//返回创建的邻居表.
nbr_entry_t* nt_create()
{
    nbr_entry_t* listHead = (nbr_entry_t*)malloc(sizeof(nbr_entry_t)*MAX_NODE_NUM);
    int listlen = 0,i,f[256];
    for(i=0; i<256; i++) f[i] = 1;
    for(i=0; i<MAX_NODE_NUM; i++){
        listHead[i].nodeID = -1;
        listHead[i].conn = -1;
    }
    int mynode = topology_getMyNodeID();
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
        if (mynode == node1 && f[node2]){
            f[node2] = 0;
            listHead[listlen].nodeID = node2;
            listHead[listlen].nodeIP = inet_addr(ip2);
            listHead[listlen].conn = -1;
            listlen++;
        }else if (mynode == node2 && f[node1]){
            f[node1] = 0;
            listHead[listlen].nodeID = node1;
            listHead[listlen].nodeIP = inet_addr(ip1);
            listHead[listlen].conn = -1;
            listlen++;
        }
    }
    fclose(fp);
    return listHead;
}

//这个函数删除一个邻居表. 它关闭所有连接, 释放所有动态分配的内存.
void nt_destroy(nbr_entry_t* nt)
{
    int i;
    for(i=0; i<MAX_NODE_NUM; i++) if (nt[i].nodeID != -1){
        if(nt[i].conn != -1) close(nt[i].conn);
    }
    return;
}

//这个函数为邻居表中指定的邻居节点条目分配一个TCP连接. 如果分配成功, 返回1, 否则返回-1.
int nt_addconn(nbr_entry_t* nt, int nodeID, int conn)
{
    int i;
    for(i=0; i<MAX_NODE_NUM; i++)if (nt[i].nodeID == nodeID){
        nt[i].conn = conn;
        return 1;
    }
    return -1;
}