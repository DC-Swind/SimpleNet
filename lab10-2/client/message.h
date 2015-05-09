#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define Tlogin '1'
#define Tacklogin '2'
#define Tuserlist '3'
#define Tgameaction '4'
#define Tgameconnect '5'
#define Ttalk '6'

#define TalkSize 256
#define MAX_THREADS 32

typedef struct Package{
    char type; //1-login,2-acklogin,3-userlist,4-gameAction,5-gameconnect
    char unuse[15];
    char content[1008]; 
}Package;

typedef struct Login{
    char username[16];
    char password[16];
}Login;

typedef struct Acklogin{
    char username[16];
    char ack;
    char content[15];
}Acklogin;

typedef struct Userlist{
    char N;
    char unuse[15];
    char content[16*MAX_THREADS];
    char status[MAX_THREADS];
}Userlist;

typedef struct Gameconnect{
    char type;  //1-A request B, 2-B recv the request , 3-B ack or not, 4-connect ans
    char game;
    char gamename[14];
    char username1[16];
    char username2[16];
    char status;  //1-success,2-failed,3-waiting...
    char first;  
    char content[14];
}Gameconnect;
typedef struct Gameaction{
    char type; //1: client to server   2: server to client
    char connID;
    char action1; //1-rock 2-scissors 3-paper
    char action2;
    char status1[6];
    char status2[6];
    char username1[16];
    char username2[16];
}Gameaction;
typedef struct Talk{
    char username[16];  //client send is to who, client recv is from who
    char content[TalkSize];
}Talk;

#endif
