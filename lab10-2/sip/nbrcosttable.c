
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "nbrcosttable.h"
#include "../common/constants.h"
#include "../topology/topology.h"
#include <sys/socket.h>
//这个函数动态创建邻居代价表并使用邻居节点ID和直接链路代价初始化该表.
//邻居的节点ID和直接链路代价提取自文件topology.dat. 
nbr_cost_entry_t* nbrcosttable_create()
{
	FILE *fp = fopen("topology/topology.dat", "r");
	int i=0;
	int top = 0;
	//for (i=0;i<MAX_NODE_NUM;i++)
		//nbr[i] = NULL;
	nbr[top].nodeID = topology_getMyNodeID();
	int id = nbr[top].nodeID;
	nbr[top].cost = 0;
	//feof(fp);
	for (i=0;i<5;i++)
	{
	  
		char ip1[20],ip2[20];
        	int len;
        	fscanf(fp, "%s", ip1);
       	 	fscanf(fp, "%s", ip2);
        	fscanf(fp, "%d", &len);
		struct in_addr addr1,addr2;
		inet_pton(AF_INET,ip1,&addr1);
		inet_pton(AF_INET,ip2,&addr2);
		
		int id1 = topology_getNodeIDfromip(&addr1);
		int id2 = topology_getNodeIDfromip(&addr2);
		
		if (id==id1)
		{
			top++;
			nbr[top].nodeID = id2;
			nbr[top].cost = len;
		}else;
		if (id==id2)
		{
			top++;
			nbr[top].nodeID = id1;
			nbr[top].cost = len;
		}else;
		//if (feof(fp)) break; else;
	}
	printf("nbr_cost_table create successful!\n");
    fclose(fp);
	return nbr;
  return 0;
} 

//这个函数删除邻居代价表.
//它释放所有用于邻居代价表的动态分配内存.
void nbrcosttable_destroy(nbr_cost_entry_t* nct)
{
	/*int i=0;
	for (i=0;i<MAX_NODE_NUM;i++)
	{
		if (nbr[i]!=NULL)
		{
			free(nbr[i]);
			nbr[i]=NULL;
		}
		else 
			break;
	}*/
  return;
}

//这个函数用于获取邻居的直接链路代价.
//如果邻居节点在表中发现,就返回直接链路代价.否则返回INFINITE_COST.
unsigned int nbrcosttable_getcost(nbr_cost_entry_t* nct, int nodeID)
{
	int i=0;
	while (i<MAX_NODE_NUM)
	{
		if (nct[i].nodeID==nodeID)
			return nct[i].cost;
		else
			i++;
	}
  return INFINITE_COST;
}

//这个函数打印邻居代价表的内容.
void nbrcosttable_print(nbr_cost_entry_t* nct)
{
	int i=0;
	int ID = topology_getMyNodeID();
	int num = topology_getNbrNum();
	while (i<num)
	{
		printf("%d to %d , cost is %d\n",ID,nct[i].nodeID,nct[i].cost);
		i++;
	}
  return;
}
