#include<stdio.h>
#include<string.h>
#include"Show.h"
#include"screen.h"
#include"commondef.h"

void main_show(int loc)
{
	clear_r();
	//printf("one\n");
	print("daxing youxi duizhan pingtai",3);
	//printf("two");
	print("game one  ",6); if (loc == 0)  append("<--",6);else ; 
	print("game two  ",7); if (loc == 1)  append("<--",7);else ;
	//printf("three");
	print("state  ",8); if (loc == 2)  append("<--",8);else ;
	//printf("four");
	print("talk  ",9); if (loc == 3)  append("<--",9);else ;
	print("exit  ",10); if (loc == 4)  append("<--",10);else ;
	print("press W pr S to choose, press K to enter",21);
	//printf("five");
	output();
}

void game_request(char user[],int loc)
{
	clear_r();
	char temp[50];
	sprintf(temp,"you have a new fighting request from %s",user);
	print(temp,3);
	print("access  ",6); if (loc == 0) append("<--",6);else;
	print("refuse  ",7); if (loc == 1) append("<--",7);else;
	print("press W pr S to choose, press K to enter",21);
	output();
}

void login()
{
	printf("Please input your user name!\n");
}

void choose_one(int loc)
{
	clear_r();
	print("<--",loc);
	print("press W pr S to choose, press K to enter",21);
	output();
}

void online_friend(char user[],int n)
{
	char* point = user;
	printf("\033c");
	printf("press 'q' to exit\n");
	int i=0;
	for (i=0;i<n;i++)
	{
		printf("%s  \n",point+16*i);
	}
}

void one_playing(int loc,int myself, int comp)
{
	clear_r();
	print("Please choose strategy",1);
	print("rock  ",3); if (loc == 0)  append("<--",3);else ; 
	print("scissors  ",4); if (loc == 1)  append("<--",4);else ;
	print("paper  ",5); if (loc == 2)  append("<--",5);else ;
	print("you have 5 seconds to choose",10);
	print("press W pr S to choose, press K to enter",21);
	//print("current score is %d : %d",myself,comp);	
	output();
}

void  one_result(int result, char myself, char comp)
{
	clear_r();
	if (result==1) print("YOU WIN !",1); else ;
	if (result==0) print("YOU LOSt!",1); else ;
	char temp[50];
	sprintf(temp,"current score is %c : %c",myself,comp);
	print(temp,2);
	print("press any key to continue",4);
	output();
}

void choose_chat(int loc)
{
	clear_r();
	print("<--",loc);
	output();
}

void choose_two(int loc)
{
	clear_r();
	print("<--",loc);
	output();
}

void two_playing(int loc)
{
	clear_r();
	print("Please choose strategy",1);
	print("1  ",3); if (loc == 0)  append("<--",3);else ; 
	print("2  ",4); if (loc == 1)  append("<--",4);else ;
	print("3  ",5); if (loc == 2)  append("<--",5);else ;
	print("4  ",6); if (loc == 3)  append("<--",6);else ;
	print("5  ",7); if (loc == 4)  append("<--",7);else ;
	print("you have 5 seconds to choose",10);
	print("press W pr S to choose, press K to enter",21);
	//print("current score is %d : %d",myself,comp);	
	output();
}

void two_result(int result)
{
	clear_r();
	if (result == 1)  print("YOU WIN! ",1);else;
	if (result == 0)  print("YOU LOST!",1);else;
	if (result == -1) print("Please wait your comprtitor choose",1);else;
	output();
}

void state()
{
	clear_r();

        print("Game one:",3);
        print("this game is rock-scissors-paper    ",5);
        print("Game two:",7);
        print("this game is lucky 19th.",9);
        print("Player A and B input number beetween 1 and 5 in turn.",10);
        print("The player who inputs a number leading the sum of ",11);
	print("numbers reaching 19th will win.",12);
        print("But the player whose input lead the sum over 19th will lose.",13);

	print("press any key to continue",20);
	output();
}
