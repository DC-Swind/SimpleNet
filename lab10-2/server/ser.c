#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ser.h"
#include "func.h"
#include "message.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "stcp_server.h"
struct Thread threads[MAX_THREADS];
struct Gaming gamings[MAX_THREADS];
struct Mailbox mailbox[MAX_THREADS];
int userlistChanged;
int threadsN;
int sockfd;
//mutex for struct Thread, threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexuserlist = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexgaming = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexmailbox = PTHREAD_MUTEX_INITIALIZER;

#define SERVERPORT0 185
#define SERVERPORT1 186
#define SERVERPORT2 187
#define SERVERPORT3 188
int sip_conn;

int connectToSIP() {
    //你需要编写这里的代码.
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0){
        printf("create socket failed in connectToSIP().\n"); 
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, CONNECTION_IP, &addr.sin_addr);
    addr.sin_port = htons(SIP_PORT);
    int rt = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
        if (rt < 0) return -1;
    return sockfd;
}


int createServerSocket(){
    sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sockfd < 0){
        printf("create sockfd failed!\n");
        return -1;
    }else printf("create sockfd success!\n");
    /* Enable address reuse */
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); 
    
    struct sockaddr_in conn;
    conn.sin_family = AF_INET;
    conn.sin_port = htons((short)6667); 
    inet_pton(AF_INET, Host, &conn.sin_addr); 
    if (bind(sockfd,(struct sockaddr*)&conn,sizeof(conn)) < 0){
        printf("bind socket failed!\n");
        return -1;
    }
    if (listen(sockfd,10) < 0){
        printf("sockfd listen failed!\n");
        return -1;
    }    
    signal(SIGPIPE,SIG_IGN);
    return sockfd;
}
void* cThread_send(void *indext){
    int index = *((int *)indext);
    while(1){
        pthread_mutex_lock(&threads[index].mutexsend);
        pthread_mutex_lock(&mutex);
        if (threads[index].requestgame != '0'){
            //send 2     
            printf("index: %d , requestgame: %c\n",index,threads[index].requestgame);
            struct Package package;
            package.type = '5';
            struct Gameconnect *conn = (struct Gameconnect*)package.content;
            conn->type = '2';
            conn->game = threads[index].requestgame;
            strncpy(conn->username1,threads[threads[index].pkwith].user.username,16);
            if (stcp_server_send(threads[index].connsocket,(char *)&package,sizeof(struct Package))<0){
                //logout(index);
                printf("send connect handshake 2 failed! --cThread_send\n");
                return NULL;
            }
            printf("game connect handshake 2.\n");

        }
        if (threads[index].myrequestans == '1' || threads[index].myrequestans == '2'){
            //send4
            if (sendhandshake4(index,threads[index].myrequestans)<=0){
                //logout(index);
                printf("send connect handshake 4 failed! --cThread_send\n");
                return NULL;
            }
            printf("game connect handshake 4.\n");

            if (threads[index].myrequestans == '1'){
                threads[index].gaming = '1';
                //--------更新gaming结构体
                updategaming(index);
            } 

            threads[index].requestgame = '0';
            threads[index].myrequestans = '0';

            pthread_mutex_unlock(&mutex);
            userlistchanged();
            continue;
        }
        pthread_mutex_unlock(&mutex);
        pthread_mutex_lock(&mutexmailbox);
        if (mailbox[index].valid == 1){
            mailbox[index].valid = 0;
            struct Package package;
            package.type = Ttalk;
            struct Talk * talk = (struct Talk*)&package.content;
            strncpy(talk->username,mailbox[index].from,16);
            strncpy(talk->content,mailbox[index].content,TalkSize);
            printf("message is :%s\n",mailbox[index].content);
            if (stcp_server_send(threads[index].connsocket,(char *)&package,sizeof(struct Package))<0){
                printf("send talk error! --cThread_send\n");
                return NULL;
            }
            printf("send talk success.\n");

        }
        pthread_mutex_unlock(&mutexmailbox);
    }
}
void* cThread(void* indext){
    pthread_mutex_lock(&mutex);
    threadsN++;
    pthread_mutex_unlock(&mutex);
    
    int index = *((int *)indext); 
    printf("a cThread start work! index is :%d\n",index);
    //socket do not need lock, just read, and this index will not be changed
    int connsocket = threads[index].connsocket;
    //wait,login
    int waitLogin = 1;
    while(waitLogin){
        struct Package package;
        int ansn_len = stcp_server_recv(connsocket,(char *)&package,sizeof(struct Package));
        if (ansn_len <= 0){
            stcp_server_close(connsocket);
            printf("wait login recv error! exit this Thread!\n");
            return NULL;
        }
        if (package.type != Tlogin) continue;
        
        struct Package acklogin;
        acklogin.type = Tacklogin;
        struct Acklogin* ack = (struct Acklogin*)acklogin.content;
        char username[16];
        strncpy(username,((struct Login*)package.content)->username,16);
        strncpy(ack->username,username,16);

        if (checkLogin((struct Login*)package.content,index)){
            //login success
            ack->ack = '1';
            waitLogin = 0;
        }else{
            //login failed
            ack->ack = '2';
        }
        if (stcp_server_send(connsocket,(char *)&acklogin,sizeof(struct Package)) < 0){
            logout(index);
            printf("send ack login failed\n");
            return NULL;
        }
        printf("send ack login\n");
    }
    //after login
    //userlist change
    userlistchanged();
    printf("index: %d , requestgame: %c\n",index,threads[index].requestgame);
    //create a send pthread,need solve the return value.
    pthread_t sendthread;
    pthread_create(&sendthread,NULL,cThread_send,(void*)&index); 
   
    //wait-game connect
    while(1){
        struct Package package;
        int ansn_len = stcp_server_recv(connsocket,(char *)&package,sizeof(struct Package));
        if (ansn_len <= 0){
            logout(index);
            printf("recv error!---from wait-game connect\n"); 
            return NULL;
        }
        if (package.type == Tgameconnect){
            //printf("recv a game connection.\n");
            struct Gameconnect *conn = (struct Gameconnect *)package.content;
            if (conn->type == '1'){
                printf("game connect handshake 1.game is %c\n",conn->game);
                if (requestconn(index,conn->username1,conn->game) == 0){
                    //conn error send 4
                    if (sendhandshake4(index,'2')<=0){
                        logout(index);
                        printf("send handshake4 error!---from conn->type='1'\n");
                        return NULL;
                    }else printf("game connect handshake 4\n");
              
                }
                gamings[index].game = conn->game;
                gamings[threads[index].pkwith].game = conn->game;
                printf("%d - %d - game:%c\n",index,threads[index].pkwith,conn->game);
            }else if (conn->type == '3'){
                printf("game connect handshake 3.game is %c\n",gamings[index].game);
                requestack(index,conn->status);
                //send 4
                if (sendhandshake4(index,conn->status)<=0){
                    logout(index);
                    printf("send connect handshake 4 failed! --from conn->type 3\n");
                    return NULL;
                }
                printf("game connect handshake 4.\n");
       
                threads[index].requestgame = '0';
                threads[index].myrequestans = '0';
                if (conn->status == '1'){
                    //更新状态
                    threads[index].gaming = '1';
                    userlistchanged();
                    //--------更新gaming结构体
                    updategaming(index);
                }     
            }
        }else if (package.type == Tgameaction){
            //printf("recv a action\n");
            //printf("%d:pkwith %d  --game is %c\n",index,gamings[index].pkwith,gamings[index].game);
            struct Gameaction *action = (struct Gameaction*)package.content;
            //单方面做出action，等待对方action
            pthread_mutex_lock(&mutexgaming);
            gamings[index].action1 = action->action1;
            gamings[gamings[index].pkwith].action2 = action->action1;
            pthread_mutex_unlock(&mutexgaming);
            pthread_mutex_unlock(&gamings[gamings[index].pkwith].mutexwaitaction);
            //printf("%d:wake up another user\n",index);
            printf("%d:another action is %c,my action is %c\n",index,gamings[index].action2,action->action1);
            int anotheruseralive = 1;
            while(1){
                pthread_mutex_lock(&mutexgaming); 
                if (gamings[index].action2 != '0'){
                    //printf("%d:another action already have\n",index);
                    pthread_mutex_unlock(&mutexgaming);
                    break;
                }
                if (gamings[gamings[index].pkwith].valid == 0){
                    pthread_mutex_unlock(&mutexgaming);
                    anotheruseralive = 0;
                    printf("%d:other user logout.game interrupt.\n",index);
                    break;
                }
                printf("%d:wait another user.\n",index);
                pthread_mutex_unlock(&mutexgaming);
                pthread_mutex_lock(&gamings[index].mutexwaitaction);
            }
            if (anotheruseralive == 0){
                exitcurrentgame(index);
                userlistchanged();
                continue;
            }
            printf("%d:start judge.\n",index);
            //action均已经做出
            if (gamings[index].game == '1') gameOne(index);
            else if (gamings[index].game == '2') gameTwo(index);
            else{
                printf("game is not exist.game ID is %c\n",gamings[index].game);
                logout(index);
                return NULL;
            }
        }else if (package.type == Ttalk){
            printf("recv a talk.\n");
            struct Talk *talk = (struct Talk *)package.content;
            sendtalk(index,talk->username,talk->content); 
        }
    } 

 
    pthread_mutex_lock(&mutex);
    threadsN--;
    pthread_mutex_unlock(&mutex);
    return NULL; 
    //send(connsocket,(char *)&ansn,sizeof(ansn),0);
    //send(connsocket,data,courseN*100,0);
}

int acceptAthread(int connsocket){
    pthread_t userlistsend;
    pthread_create(&userlistsend,NULL,sendUserlist,NULL);  

    pthread_mutex_lock(&mutex);
    if(threadsN >= MAX_THREADS){ 
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    pthread_mutex_unlock(&mutex);

   /*     struct sockaddr_in conn;
        socklen_t addrlen = sizeof(struct sockaddr_in);
        int connsocket = accept(sockfd,(struct sockaddr *)&conn,&addrlen);
   */
    pthread_mutex_lock(&mutex);   
    int index = getThreadSlot();
    //printf("Get slot is :%d \n",index);
    threads[index].valid = 1;
    threads[index].connsocket = connsocket;
    printf("A client connected! thread slot(index) is %d\n",index);
    int *transportindex = malloc(sizeof(int));
    *transportindex = index;
    int rc =pthread_create(&(threads[index].thread),NULL,cThread,(void*)transportindex);
    pthread_mutex_unlock(&mutex);
    if (rc){
        printf("ERROR:return code from pthread_create() is %d\n",rc);
    }
    
    return 0;
}
int startRun(){
    while(1){
        pthread_t userlistsend;
        pthread_create(&userlistsend,NULL,sendUserlist,NULL);  

        pthread_mutex_lock(&mutex);
        if(threadsN >= MAX_THREADS){ pthread_mutex_unlock(&mutex); continue;}
        pthread_mutex_unlock(&mutex);

        struct sockaddr_in conn;
        socklen_t addrlen = sizeof(struct sockaddr_in);
        int connsocket = accept(sockfd,(struct sockaddr *)&conn,&addrlen);

        pthread_mutex_lock(&mutex);   
        int index = getThreadSlot();
        threads[index].valid = 1;
        threads[index].connsocket = connsocket;
        printf("A client connected! thread is %d\n",index);
        int rc =pthread_create(&(threads[index].thread),NULL,cThread,(void*)&index);
        pthread_mutex_unlock(&mutex);
        if (rc){
            printf("ERROR:return code from pthread_create() is %d\n",rc);
        }
    }
    return 0;
}
/*
int readCourseList(){
    FILE *fp;
    if((fp=fopen("courselist.txt","r"))==NULL){
        printf("Cannot open file strike any key exit!");
        exit(1);
    }
    databaseN = 0;   
    while(fgets((char *)database[databaseN].Name,sizeof(database[databaseN].Name),fp) != NULL){
        fgets((char *)database[databaseN].Class,sizeof(database[databaseN].Class),fp);
        database[databaseN].Grade = fgetc(fp);
        fgetc(fp);
        fgets((char *)database[databaseN].Classroom,sizeof(database[databaseN].Classroom),fp);
        database[databaseN].Day = fgetc(fp);
        fgetc(fp);
        database[databaseN].TimeStart = fgetc(fp);
        fgetc(fp);
        database[databaseN].TimeEnd = fgetc(fp);
        fgetc(fp);
        //printf("%s,%s,%c,%s,%c,%c,%c\n",database[databaseN].Name,database[databaseN].Class,database[databaseN].Grade,database[databaseN].Classroom,database[databaseN].Day,database[databaseN].TimeStart,database[databaseN].TimeEnd);
        databaseN++;
    }
    
    fclose(fp);
    return 0;
}
*/
int server(){
    init();
    //if (createServerSocket() < 0 ) return 0;
    startRun();    
    return 0;
}

void* serverlisten(void* i){
    int index = *((int*)i);
    if (index == 0){ 
        while(1){
            int sockfd= stcp_server_sock(SERVERPORT0);
            if(sockfd<0) {
                printf("can't create stcp server\n");
                exit(1);
            }
            stcp_server_accept(sockfd);
            acceptAthread(sockfd);
            while(TCB[sockfd]->state != CLOSED){
                sleep(1);
            }
        }
    }else if (index == 1){
        while(1){
            int sockfd= stcp_server_sock(SERVERPORT1);
            if(sockfd<0) {
                printf("can't create stcp server\n");
                exit(1);
            }
            stcp_server_accept(sockfd);
            acceptAthread(sockfd);
            while(TCB[sockfd]->state != CLOSED){
                sleep(1);
            }
        }
    }else if (index == 2){
        while(1){
            int sockfd= stcp_server_sock(SERVERPORT2);
            if(sockfd<0) {
                printf("can't create stcp server\n");
                exit(1);
            }
            stcp_server_accept(sockfd);
            acceptAthread(sockfd);
            while(TCB[sockfd]->state != CLOSED){
                sleep(1);
            }
        }
    }else if (index == 3){
        while(1){
            int sockfd= stcp_server_sock(SERVERPORT3);
            if(sockfd<0) {
                printf("can't create stcp server\n");
                exit(1);
            }
            stcp_server_accept(sockfd);
            acceptAthread(sockfd);
            while(TCB[sockfd]->state != CLOSED){
                sleep(1);
            }
        }
    }else;
    return NULL;
}
int main(){
    sip_conn = connectToSIP();
    if(sip_conn<0) {
        printf("can not connect to the local SIP process\n");
    }

    init();
    
    //初始化STCP服务器
    stcp_server_init(sip_conn);

    int* i = malloc(sizeof(int)); *i = 0;
    int* j = malloc(sizeof(int)); *j = 1;
    int* k = malloc(sizeof(int)); *k = 2;
    int* l = malloc(sizeof(int)); *l = 3;
    
    pthread_t serverlisten1;
    pthread_create(&serverlisten1,NULL,serverlisten,(void*)i);  
    
    pthread_t serverlisten2;
    pthread_create(&serverlisten2,NULL,serverlisten,(void*)j);
    
    pthread_t serverlisten3;
    pthread_create(&serverlisten3,NULL,serverlisten,(void*)k);
    
    pthread_t serverlisten4;
    pthread_create(&serverlisten4,NULL,serverlisten,(void*)l);
    
/*    
    //在端口SERVERPORT1上创建STCP服务器套接字 
    int sockfd= stcp_server_sock(SERVERPORT1);
    if(sockfd<0) {
        printf("can't create stcp server\n");
        exit(1);
    }
    //监听并接受来自STCP客户端的连接 
    stcp_server_accept(sockfd);
    acceptAthread(sockfd);
    
    //在端口SERVERPORT2上创建另一个STCP服务器套接字
    int sockfd2= stcp_server_sock(SERVERPORT2);
    if(sockfd2<0) {
        printf("can't create stcp server\n");
        exit(1);
    }
    //监听并接受来自STCP客户端的连接 
    stcp_server_accept(sockfd2);
    acceptAthread(sockfd2);
*/    
    //server();
    while(1){
        sleep(60);
    }
    return 0;
}
