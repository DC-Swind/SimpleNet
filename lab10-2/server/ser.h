#ifndef __SER_H__
#define __SER_H__

//#define Host "127.0.0.1"
#define Host "192.168.1.100"


#include <pthread.h>
#include "message.h"

struct Thread{
    pthread_t thread;
    int connsocket;
    int valid;
    struct Login user;
    int logined;  //1-logined , 0- unlogined
    char requestgame;//0-no request, 1-game1
    char gaming; //0-free,1-game1,...
    int pkwith; //-1 - free, >=0 - with index
    char myrequestans;//1-success,2-failed,3-waiting
    pthread_mutex_t mutexsend;
};
struct Gaming{
    int valid;
    char game;
    int pkwith;
    char action1;
    char action2;
    char status1;
    char status2;
    char username1[16];
    char username2[16];
    int sum;
    pthread_mutex_t mutexwaitaction;
};
struct Mailbox{
    int valid;
    char from[16];
    char content[TalkSize];
};
extern int threadsN;
extern struct Thread threads[MAX_THREADS];
extern struct Gaming gamings[MAX_THREADS];
extern struct Mailbox mailbox[MAX_THREADS];
extern pthread_mutex_t mutex;
extern pthread_mutex_t mutexuserlist;
extern pthread_mutex_t mutexgaming;
extern pthread_mutex_t mutexmailbox;
extern int userlistChanged;

#endif
