#ifndef LIST_TIMER
#define LIST_TIMER
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <time.h>

class util_timer;

struct client_data {
    sockaddr_in addr;   //客户的地址
    int sockfd;        //客户的socket
    util_timer *timer; //定时器
};

class util_timer {
public:
    util_timer():pre(nullptr), next(nullptr){}

    time_t expire;
    client_data *user_data; //定时器包含的客户数据
    void (*back_fun) (client_data*); //超时回调函数
    util_timer* pre;
    util_timer* next;
};

//基于升序链表的定时器
class sort_timer_list {
public:
    sort_timer_list();
    ~sort_timer_list();

    void add_timer(util_timer* timer);
    void adjust_timer(util_timer* timer);
    void del_timer(util_timer* timer);
    void tick();
private:
    void add_timer(util_timer* timer, util_timer* begin);

    util_timer* head;
    util_timer* tail;
};


class Utils {
public:
    static int *u_pipefd;
    sort_timer_list m_timer_list;
    static int u_epollfd;
    int m_TIMESLOT;

    Utils(){};
    ~Utils(){};
    void init(int timeslot);    //初始化工具类
    int setnonblocking(int fd);    //设置非阻塞
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode); //注册内核事件
    static void sig_handler(int sig);                      //信号处理函数 一定要是静态的
    void add_sig(int sig, void (*handler)(int), bool restart = true);    //注册信号处理函数
    void timer_handler();   //信号处理函数会调用该函数？
    void show_error(int connfd, const char *info);
};

void back_fun(client_data* user_data);



#endif