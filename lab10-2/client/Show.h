#ifndef SHOW_H
#define SHOW_H

void main_show(int loc); //id = 1
void game_request(char user[],int loc); //id = 2
void login();  
void choose_one(int loc);  //id = 3
void online_friend(char user[], int n);
void one_playing(int loc,int myself,int comp); // id = 4
void one_result(int result,char myself, char comp); // id = 5
void choose_chat(int loc); //id = 6
void choose_two(int loc); //id = 7
void two_playing(int loc); //id = 8
void two_result(int result); // id = 9
void state();


#endif

