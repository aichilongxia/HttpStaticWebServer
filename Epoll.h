/*
 * Author: Broglie 
 * E-mail: yibo141@outlook.com
 */

#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <map>
#include <unordered_map>
#include "Handler.h"
#include <sys/timerfd.h>
#include "Time.h"
#include "HeapTime.h"

#define FDSIZE 1000
#define FD_LIMIT 5000
#define TIMESLOT 5

//接收信号使用的管道
//extern int pipefd[2];
class EventLoop;
class Epoll 
{
public:
    //friend void cb_func( client_data* user_data );
    //friend void sig_handler( int sig );
    // static void sig_handler( int sig )
    // {
    //     int save_errno = errno;
    //     int msg = sig;
    //     send( pipefd[1], ( char* )&msg, 1, 0 );
    //     errno = save_errno;
    // }
    static void cb_func( client_data* user_data)
    {
        epoll_ctl(user_data->epollfd,  EPOLL_CTL_DEL, user_data->sockfd, 0 );
        assert( user_data );
        close( user_data->sockfd );
        printf( "close fd %d\n", user_data->sockfd );
    }
    // void addsig( int sig )
    // {
    //     struct sigaction sa;
    //     memset( &sa, '\0', sizeof( sa ) );
    //     sa.sa_handler = sig_handler;//(this->sig_handler);//(void (Epoll::*)(int))
    //     sa.sa_flags |= SA_RESTART;
    //     sigfillset( &sa.sa_mask );
    //     assert( sigaction( sig, &sa, NULL ) != -1 );
    // }
    void timer_handler()
    {
        if(timer_lst->size() != 0)
        {
            std::cout<<" tick here "<<std::endl;
            timer_lst->tick();
            //alarm( timer_lst->TopTime() );//第一次到时间后，还需重新定时
            new_value.it_value.tv_sec = timer_lst->TopTime(); //重新定时的时间
            new_value.it_value.tv_nsec = 0; 
	        int ret = timerfd_settime(timefd, 0, &new_value, NULL);//启动定时器
            assert(ret != -1);
        }        
    }
    //构造函数
    explicit Epoll(EventLoop *loop)
        :ownerLoop(loop),
         epollfd(epoll_create1(EPOLL_CLOEXEC)),
         retEvents(FDSIZE) // 初始时为epoll_wait预留8个返回的epoll_event
    { 
        timer_lst = new HeapTimer;
        users = new client_data[FD_LIMIT]; 
        timeout = false;
        // FIXME: 调用日志库写入日志并终止程序
        if(epollfd < 0)
            std::cout << "Epoll::epoll_create1() error: " << ::strerror(errno) << std::endl;
        int ret = clock_gettime(CLOCK_REALTIME, &now);//获取时钟时间
        assert(ret != -1);
        new_value.it_value.tv_sec = TIMESLOT; //第一次到期的时间
        new_value.it_value.tv_nsec = now.tv_nsec; 
        new_value.it_interval.tv_sec = 1;      //之后每次到期的时间间隔
        new_value.it_interval.tv_nsec = 0;

        timefd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK); // 构建了一个定时器
        assert(timefd != -1);

        ret = timerfd_settime(timefd, 0, &new_value, NULL);//启动定时器
        assert(ret != -1);
        addToEpollTimerFd(timefd);
        // add all the interesting signals here
        //addsig( SIGALRM );
    }

    ~Epoll()
    {
        removeFd(timefd); 
        close(timefd);
        close(epollfd);
        delete []users;
        delete timer_lst;
        connfdHash.clear();
    }

    int getEpollFd() {return epollfd;}
    //int getPipeFd1() {return pipefd[1];}
    // 调用epoll_wait，并将其交给Event类的handleEvent函数处理
    void epoll(std::vector<Handler*> &events);
    void removeFd(const int fd);
    /*
    void assertInLoopThread() const 
    {
        ownerLoop->assertInLoopThread();
    }
    */
    void addToEpoll(const int fd);
    void addToEpollTimerFd(const int fd);
private: 
    int timefd; 
    struct itimerspec new_value;
    struct timespec now;
    //小根堆定时器
    HeapTimer *timer_lst;
    //一个epollfd在timer_lst堆中所能连接的客户
    client_data* users;
    //是否超时时间到达
    bool timeout;
    //记录connfd是否已在哈希表中
    std::unordered_map<int,int> connfdHash;
    EventLoop *ownerLoop;
    int epollfd;
    //epollwait响应的事件
    std::vector<struct epoll_event> retEvents;

};

#endif // EPOLL_H