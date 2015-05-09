#include<stdio.h>
#include"screen.h"
#include"logic.h"
#include"commondef.h"


char* line[24];
int tail[24];

void init_screen()
{
	//24*80
	int i=0;
	for (i=0;i<24*80-1;i++)
	{
		scr[i]=' ';
	}
	for (i=0;i<24;i++)
	{
		line[i] = scr+i*80+20;
		tail[i] = 0;
	}
	scr[22*80] = '\0';
}

void print(char data[], int l)
{
	int i=0;
	while (data[i]!='\0' && i<60)
	{	
		(line[l]+i)[0] = data[i]; 
		i++;
	}
	//strncpy(data[i],60);
	
	tail[l] = i;
	while (i<60)
	{
		(line[l]+i)[0] = ' '; 
		i++;
	}
}

void clear_r()
{
	int i=0;
	for (i=0;i<22;i++)
		print(" ",i);
}

void clear_l()
{
	char* tpoint[20];
	int i,j;
	for(i=0;i<20;i++)
	{
		tpoint[i] = scr + 80*i;
		for (j=0;j<20;j++)
			(tpoint[i]+j)[0] = ' ';
	}
}

void append(char data[], int l)
{
	int i=tail[l];
	while (data[i-tail[l]]!='\0' && i<60)
	{	
		(line[l]+i)[0] = data[i-tail[l]]; 
		i++;
	}
	tail[l] = i;
	while (i<60)
	{
		(line[l]+i)[0] = ' '; 
		i++;
	}
}

void user_change(char user[],char status[],int num)
{
	clear_l();
	int i=0;
	for (i=0;i<num;i++)
	{	
		char* point = user + i*16;
		int j=0;
		while (point[j]!='\0' && j<16)
		{
			(line[i]-20+j)[0] = point[j];
			j++;
		}
		(line[i]-20+j)[0] = ' ';
		j++;
		if (status[i] == '1')
		{
			(line[i]-20+j)[0] = 'p';
			j++;
			(line[i]-20+j)[0] = 'l';
			j++;
			(line[i]-20+j)[0] = 'a';
			j++;
			(line[i]-20+j)[0] = 'y';
			j++;
			(line[i]-20+j)[0] = 'i';
			j++;
			(line[i]-20+j)[0] = 'n';
			j++;
			(line[i]-20+j)[0] = 'g';
			j++;
		}else 
		{
			(line[i]-20+j)[0] = status[i];
			j++;
		}
		while (j<20)
		{
			(line[i]-20+j)[0] = ' ';
			j++;
		}
	}
	output();
	printf("USERLIST! 9");
}


void output()
{
	printf("\033c");	
	int i=0;
	for (i=0;i<22*80;i++)
	{
		printf("%c",scr[i]);
	}
	//printf("%s",scr);
}
