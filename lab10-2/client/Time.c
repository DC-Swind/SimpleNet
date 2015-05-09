#include<unistd.h>
#include<time.h>
#include<stdlib.h>

extern int countdown;

void *time_3(void *threadid)
{
	countdown = 3;
	int i;
	for (i=0;i<3;i++)
	{
		if (countdown!=3) return NULL;else;
		sleep(1);
	}
	countdown = 0;
	return NULL;
}

void *time_5(void *threadid)
{
	countdown = 5;
	int i;
	for (i=0;i<5;i++)
	{
		if (countdown!=5) return NULL;else;
		sleep(1);
	}
	countdown = 0;
	return NULL;
}

int random_3()
{
	srand(time(NULL));
	int temp=1;
 	temp=rand()%3;
	return temp;
}
