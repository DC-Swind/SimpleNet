#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "../common/constants.h"
#include "../topology/topology.h"
#include "routingtable.h"

//makehash()是由路由表使用的哈希函数.
//它将输入的目的节点ID作为哈希键,并返回针对这个目的节点ID的槽号作为哈希值.
int makehash(int node)
{
	return node%MAX_ROUTINGTABLE_SLOTS;
  return 0;
}

//这个函数动态创建路由表.表中的所有条目都被初始化为NULL指针.
//然后对有直接链路的邻居,使用邻居本身作为下一跳节点创建路由条目,并插入到路由表中.
//该函数返回动态创建的路由表结构.
routingtable_t* routingtable_create()
{
	routingtable = malloc(sizeof(routingtable_t));
	int i;
	for (i=0;i<MAX_ROUTINGTABLE_SLOTS;i++)
		routingtable->hash[i] = NULL;
	
	FILE *fp = fopen("topology/topology.dat", "r");
	int id = topology_getMyNodeID();
	//feof(fp);
	for (int i=0;i<5;i++)
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

		if (id1 == id)
		{
			int mh = makehash(id2);
			routingtable_entry_t* point = routingtable->hash[mh];
			if (point != NULL)
				while (point->next!=NULL) point = point->next;
			else;
			
			routingtable_entry_t* Node = malloc(sizeof(routingtable_entry_t));
			Node->destNodeID = id2;
			Node->nextNodeID = id2;
			Node->next = NULL;
			
			if (point != NULL)
				point->next = Node;
			else
				routingtable->hash[mh] = Node;	
		}else;
		if (id2 == id)
		{
			int mh = makehash(id1);
			routingtable_entry_t* point = routingtable->hash[mh];
			if (point != NULL)
				while (point->next!=NULL) point = point->next;
			else;
			
			routingtable_entry_t* Node = malloc(sizeof(routingtable_entry_t));
			Node->destNodeID = id1;
			Node->nextNodeID = id1;
			Node->next = NULL;
			
			if (point != NULL)
				point->next = Node;
			else
				routingtable->hash[mh] = Node;	
		}else;
		//if (feof(fp)) break; else;
			
	}
	fclose(fp);
	printf("routing_table create successful!\n");
	return routingtable;
	
  //return 0;
}

//这个函数删除路由表.
//所有为路由表动态分配的数据结构将被释放.
void routingtable_destroy(routingtable_t* routingtable)
{
	int i=0;
	while (i<MAX_ROUTINGTABLE_SLOTS)
	{
		if (routingtable->hash[i]==NULL)
			i++;
		else
		{
			if (routingtable->hash[i]->next==NULL)
			{
				free(routingtable->hash[i]);
				i++;
			}else
			{
				routingtable_entry_t* point = routingtable->hash[i];
				while (point->next->next==NULL) point = point->next;
				free(point->next);
				point->next = NULL;
			}
		}			
	}
	free(routingtable);
	routingtable = NULL;
  return;
}

//这个函数使用给定的目的节点ID和下一跳节点ID更新路由表.
//如果给定目的节点的路由条目已经存在, 就更新已存在的路由条目.如果不存在, 就添加一条.
//路由表中的每个槽包含一个路由条目链表, 这是因为可能有冲突的哈希值存在(不同的哈希键, 即目的节点ID不同, 可能有相同的哈希值, 即槽号相同).
//为在哈希表中添加一个路由条目:
//首先使用哈希函数makehash()获得这个路由条目应被保存的槽号.
//然后将路由条目附加到该槽的链表中.
void routingtable_setnextnode(routingtable_t* routingtable, int destNodeID, int nextNodeID)
{
	int mh = makehash(destNodeID);

	routingtable_entry_t* point = routingtable->hash[mh];
	if (point != NULL)
	{
		int judge = 0;
		while (judge == 0 ) 
		{
			if (point->destNodeID==destNodeID)
				judge = 1;
			else
			  if (point->next==NULL)
			    break;
			  else
				point = point->next;
		}
		if (judge)
		{
			point->nextNodeID = nextNodeID;
		}else
		{
			routingtable_entry_t* Node = malloc(sizeof(routingtable_entry_t));
			Node->destNodeID = destNodeID;
			Node->nextNodeID = nextNodeID;
			Node->next = NULL;
			point = routingtable->hash[mh];
			while (point->next!=NULL) point = point->next;
			point->next = Node;
			
		}
	}
	else	
	{
		routingtable_entry_t* Node = malloc(sizeof(routingtable_entry_t));
		Node->destNodeID = destNodeID;
		Node->nextNodeID = nextNodeID;
		Node->next = NULL;
		routingtable->hash[mh] = Node;
	}
			

  return;
}

//这个函数在路由表中查找指定的目标节点ID.
//为找到一个目的节点的路由条目, 你应该首先使用哈希函数makehash()获得槽号,
//然后遍历该槽中的链表以搜索路由条目.如果发现destNodeID, 就返回针对这个目的节点的下一跳节点ID, 否则返回-1.
int routingtable_getnextnode(routingtable_t* routingtable, int destNodeID)
{
	int mh = makehash(destNodeID);
	routingtable_entry_t* point = routingtable->hash[mh];
	while (point != NULL)
	{
		if (point->destNodeID==destNodeID)
			return point->nextNodeID;
		else
			point = point->next;
	}
	return -1;
  return 0;
}

//这个函数打印路由表的内容
void routingtable_print(routingtable_t* routingtable)
{
	int i;
	for (i=0;i<MAX_ROUTINGTABLE_SLOTS;i++)
	{
		routingtable_entry_t* point = routingtable->hash[i];
		while (point!=NULL)
		{
			printf("destNodeID: %d, nextNodeID: %d\n",point->destNodeID,point->nextNodeID);
			point = point->next;
		}
	}	
  return;
}
