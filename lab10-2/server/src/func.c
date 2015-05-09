#include <string.h>
#include "ser.h"
#include "func.h"
#include "message.h"
//#include <sys/socket.h>
#include <stdio.h>
#include <stcp_server.h>
#include <unistd.h>

int getThreadSlot(){
    //init put there , re fenpei data rewirte
    int i;
    for (i=0; i<MAX_THREADS; i++) if(threads[i].valid == 0){
        threads[i].requestgame = '0';
        threads[i].logined = 0;
        threads[i].gaming = '0';
        pthread_mutex_unlock(&threads[i].mutexsend);
        return i;
    }
    return -1;
}

void init(){
    int i;
    for (i=0; i<MAX_THREADS; i++){
        threads[i].valid = 0;
        threads[i].logined = 0;
        threads[i].gaming = '0';
    }
    threadsN = 0;
    userlistChanged = 0;
}

int datacpy(char *a, char *b, int size){
    int i;
    for (i = 0; i<size; i++) a[i] = b[i];
    return 0;
}

int contains(char *a,char *b){
    int lena = strlen(a);
    int lenb = strlen(b);
    if (lena < lenb) return 0;
    int i;
    for (i = 0; i<=lena - lenb; i++){
        char *p = a+i;
        int j=0;
        int flag = 1;
        for (j = 0; j< lenb; j++) if (p[j] != b[j]){
            flag = 0;
        }
        if (flag) return 1;
    }
    return 0;
}

int checkLogin(struct Login* login,int index){
    pthread_mutex_lock(&mutex);
    int i;
    for(i=0; i<MAX_THREADS; i++)if (threads[i].valid && threads[i].logined){
        int len=strlen(login->username);
        if (len != strlen(threads[i].user.username)) continue;
        if(strncmp(login->username,threads[i].user.username,len) == 0){
            printf("%s login failed!\n",login->username);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
    strncpy((char*)&threads[index].user,(char*)login,sizeof(struct Login));
    threads[index].logined = 1;
    printf("%s login success!\n",login->username);
    pthread_mutex_unlock(&mutex);
    return 1;
}

void *sendUserlist(){
    while(1){
        pthread_mutex_lock(&mutexuserlist);
        pthread_mutex_lock(&mutex);
        if (userlistChanged == 0){
            pthread_mutex_unlock(&mutex);
            continue;
        }
        //send userlist to everyone
        int i,j=0;
        struct Package package;
        package.type = '3';
        struct Userlist* userlist = (struct Userlist*)package.content;
        userlist->N = '0';
        for(i=0; i<MAX_THREADS;i++)if(threads[i].valid == 1 && threads[i].logined == 1){
            strncpy(&(userlist->content[j*16]),threads[i].user.username,16);
            userlist->status[j] = threads[i].gaming;
            userlist->N++;
            j++;           
        }
        for(i=0; i<MAX_THREADS;i++)if(threads[i].valid == 1 && threads[i].logined == 1){
            j = stcp_server_send(threads[i].connsocket,(char *)&package,sizeof(struct Package));
            if (j<=0){ 
                printf("send userlist occurs error! to %s\n",threads[i].user.username);
            }
            printf("send userlist.\n");
        }
        userlistChanged = 0; 
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
int userlistchanged(){
    pthread_mutex_lock(&mutex);
    userlistChanged = 1;
    pthread_mutex_unlock(&mutex);
    pthread_mutex_unlock(&mutexuserlist);
    return 0;
}

int requestconn(int a,char *bname,char game){
    pthread_mutex_lock(&mutex);
    int i;
    int b = -1;
    for(i=0; i<MAX_THREADS;i++)if (threads[i].logined == 1){
        int len=strlen(threads[i].user.username);
        if (len != strlen(bname)) continue;
        if(strncmp(bname,threads[i].user.username,len) == 0){
            b = i;
            break;
        }
    }
    //如果没有b，或者有人申请我，那么我不能申请别人
    if (a == b || b == -1 || threads[a].requestgame != '0' || threads[b].gaming != '0'){
        printf("can not pk with %s. %c-%c\n",bname,threads[a].requestgame,threads[b].gaming);
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    else{
        if (threads[b].requestgame == '0'){
            threads[b].requestgame = game;
            threads[b].pkwith = a;
            threads[a].pkwith = b;
            pthread_mutex_unlock(&threads[b].mutexsend);
            pthread_mutex_unlock(&mutex);
            return 1;
        }
        pthread_mutex_unlock(&mutex);
        printf("unbelievable! -- from requestconn\n");
        return 0;
    }
}
int requestack(int index,char status){
    pthread_mutex_lock(&mutex);
    int b = threads[index].pkwith;
    threads[b].myrequestans = status;
    //threads[b].pkwith = index;
    threads[index].requestgame = '0';
    threads[index].gaming = '0';
    pthread_mutex_unlock(&threads[b].mutexsend);
    pthread_mutex_unlock(&mutex);
    return 0;
}
int sendhandshake4(int index,char status){
    struct Package package;
    package.type = '5';
    struct Gameconnect *conn = (struct Gameconnect*)package.content;
    conn->type = '4';
    conn->status = status;
    conn->game = gamings[index].game;
    printf("game is %c\n",conn->game);
    conn->first = (index > threads[index].pkwith?'1':'0');
    
    int ans = stcp_server_send(threads[index].connsocket,(char *)&package,sizeof(struct Package));
    return ans;
}
int gameOne(int index){
    int win = 0;
    int action1 = (int)gamings[index].action1 - 48;
    int action2 = (int)gamings[index].action2 - 48;
    if (action1 == action2) win = 0;
    else if (action1 % 3 + 1 == action2) win = 1;
    else win = 2;
    if (win == 1) gamings[index].status2--;else
    if (win == 2) gamings[index].status1--;
    gamings[index].action1 = '0';
    gamings[index].action2 = '0'; 
    //send package;
    struct Package package;
    package.type = '4';
    struct Gameaction *action = (struct Gameaction*)package.content;
    action->type = '2'; 
    action->action1 = gamings[index].action1;
    action->action2 = gamings[index].action2;
    action->status1[0] = gamings[index].status1;
    action->status2[0] = gamings[index].status2;
    strncpy(action->username1,gamings[index].username1,16);
    strncpy(action->username2,gamings[index].username2,16);
    if (stcp_server_send(threads[index].connsocket,(char*)&package,sizeof(struct Package))<=0){
        printf("send action from server to client failed inf gameone.\n");
    }
    if (gamings[index].status1 == '0' || gamings[index].status2 == '0'){ 
        endgame(index);
        userlistchanged();

    }
    return 0;
}
int endgame(int index){
    printf("%d end game.\n",index);
    threads[index].gaming = '0';
    threads[index].pkwith = -1;
    return 0;
}
int gameTwo(int index){
    int action1 = (int)gamings[index].action1 - 48;
    int action2 = (int)gamings[index].action2 - 48;
    gamings[index].action1 = '0';
    gamings[index].action2 = '0';

    struct Package package;
    package.type = '4';
    struct Gameaction *action = (struct Gameaction*)package.content;
    action->type = '2';
    action->status1[0] = gamings[index].status1;
    action->status2[0] = gamings[index].status2;

    if (action1 == 9){
        action->action1 = '1';
        gamings[index].sum += action2;
        if (gamings[index].sum == 19){
            action->status1[0] = '0';
            action->action1 = '0';
            endgame(index);
            userlistchanged();
        }
        if (gamings[index].sum > 19){
            action->status2[0] = '0';
            action->action1 = '0'; 
            endgame(index);
            userlistchanged();
        }
    }else{
        action->action1 = '0';
        gamings[index].sum += action1; 
        if (gamings[index].sum == 19){
            action->status2[0] = '0';
            endgame(index);
            userlistchanged();
        }
        if (gamings[index].sum > 19){
            action->status1[0] = '0';
            endgame(index);
            userlistchanged();
        }
    }
    

    strncpy(action->username1,gamings[index].username1,16);
    strncpy(action->username2,gamings[index].username2,16);
    if (stcp_server_send(threads[index].connsocket,(char*)&package,sizeof(struct Package))<=0){
        printf("send action from server to client failed in gametwo.\n");
    }
    return 0; 
}
int updategaming(int index){
    gamings[index].valid = 1;
    gamings[index].pkwith = threads[index].pkwith;
    gamings[gamings[index].pkwith].valid = 1;
    strncpy(gamings[index].username1,threads[index].user.username,16);
    strncpy(gamings[index].username2,threads[threads[index].pkwith].user.username,16);
    
    if (gamings[index].game == '1'){
        gamings[index].status1 = '3';
        gamings[index].status2 = '3';
    }else if (gamings[index].game == '2'){
        gamings[index].status1 = '1';
        gamings[index].status2 = '1';
        gamings[index].sum = 0;
    }
    gamings[index].action1 = '0';
    gamings[index].action2 = '0';
    return 0;
}
int logout(int index){
    //change 2 struct in ser.h
    //close socket
    //N--
    close(threads[index].connsocket);
    pthread_mutex_lock(&mutex);
    threads[index].valid = 0;
    threads[index].logined = 0;
    threads[index].requestgame = '0';
    threads[index].gaming = '0';
    threads[index].pkwith = -1;
    threads[index].myrequestans = 0;
    threadsN--;
    pthread_mutex_unlock(&mutex);
    pthread_mutex_lock(&mutexgaming);
    gamings[index].valid = 0;
    gamings[index].game = '0';
    pthread_mutex_unlock(&gamings[gamings[index].pkwith].mutexwaitaction);
    gamings[index].pkwith = -1;
    pthread_mutex_unlock(&mutexgaming);
    printf("%s logout.\n",threads[index].user.username);
    userlistchanged();
    return 0;
}

int sendtalk(int index,char*to,char *content){
    int i,b=-1;
    for (i=0; i<MAX_THREADS;i++) if (strncmp(to,threads[i].user.username,16)==0){
        b = i;
        break;
    }
    if (b == -1) return 0;
    pthread_mutex_lock(&mutexmailbox);
    mailbox[b].valid = 1;
    strncpy(mailbox[b].content,content,TalkSize);
    strncpy(mailbox[b].from,threads[index].user.username,16);
    pthread_mutex_unlock(&mutexmailbox);
    pthread_mutex_unlock(&threads[b].mutexsend);
    return 0;
}
int exitcurrentgame(int index){
    threads[index].gaming = '0';
    threads[index].pkwith = -1;
    gamings[index].valid = 0;
    gamings[index].game = '0';
    gamings[index].pkwith = -1;
    
    struct Package acklogin;
    acklogin.type = Tacklogin;
    struct Acklogin* ack = (struct Acklogin*)acklogin.content;
    ack->ack = '1';
    
    if (stcp_server_send(threads[index].connsocket,(char *)&acklogin,sizeof(struct Package))<0){
        printf("send ack login failed -- anotheruseralive\n");
    }
    return 0;
}
