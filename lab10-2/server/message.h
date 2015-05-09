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


struct Package{
    char type; //1-login,2-acklogin,3-userlist,4-gameAction,5-gameconnect
    char unuse[15];
    char content[1008]; 
};
struct Login{
    char username[16];
    char password[16];
};

struct Acklogin{
    char username[16];
    char ack;   //1-success, 2-failed
    char content[15];
};

struct Userlist{
    char N;
    char unuse[15];
    char content[16*MAX_THREADS];
    char status[MAX_THREADS];
};

struct Gameconnect{
    char type;  //1-A request B, 2-B recv the request , 3-B ack or not, 4-connect ans
    char game;
    char gamename[14];
    char username1[16];
    char username2[16];
    char status;  //1-success,2-failed,3-waiting...
    char first;
    char content[14];
};
struct Gameaction{
    char type;  //1-C to S, 2- S to C
    char connID;
    char action1;
    char action2;
    char status1[6];
    char status2[6];
    char username1[16];
    char username2[16];
};
struct Talk{
    char username[16];  //client send is to who, client recv is from who
    char content[TalkSize];
};
#endif
