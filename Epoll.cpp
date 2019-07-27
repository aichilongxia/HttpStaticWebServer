/*
 * Author: Broglie 
 * E-mail: yibo141@outlook.com
 */

#include "Epoll.h"

void Epoll::epoll(std::vector<Handler*> &activeEvents)
{
    //std::cout<<" stun in before epoll_wait"<<std::endl;
    int numEvents = epoll_wait(epollfd,
                                 retEvents.data(),
                                 static_cast<int>(retEvents.size()),
                                 0);// 每秒返回一次
    // std::cout<<"numEvents size = "<< numEvents <<std::endl;
    if(numEvents < 0) 
    {
        if(errno != EINTR)
        {   // FIXME: 调用日志库写入日志并终止程序
            std::cout << "Epoll::epoll() error: " << strerror(errno) << std::endl;
            exit(1);
        }
    }
    else if(numEvents == 0) 
    {
        // 什么都不做
    }
    else 
    {
        //std::cout<<"stun in after epoll_wait"<<std::endl;
        /*
        if(numEvents == retEvents.size())
            retEvents.resize(1.5 * numEvents);
        */
        std::cout<<"ret size = "<<retEvents.size()<<std::endl;
        std::cout<<"numEvents size = "<< numEvents <<std::endl;
        // 活动的fd可能是 1、信号，2、长连接fd，3、短链接fd。 首先处理信号，然后将长短连接分别处理。
        for(int i = 0; i < numEvents; ++i) 
        {    
            int sockFd = retEvents[i].data.fd;
            std::cout<<"socketfd = "<< sockFd << std::endl;
            if( (sockFd == timefd) && (retEvents[i].events & EPOLLIN) )
            {
                uint64_t exp;
                ssize_t s = read(sockFd, &exp, sizeof(uint64_t));
                std::cout << "signal deals" << std::endl;
                timeout = true;
                continue;
            }
            Handler *h = new Handler(sockFd);
            activeEvents.push_back(h);
        }

        for(std::vector<Handler*>::iterator iter = activeEvents.begin();
                iter != activeEvents.end(); ++iter)
        {
            std::cout << "----------Handle request----------" << std::endl;
            // 处理客户端请求的入口,采解析
            //(*iter)->handle();
            // 长连接第一次加入的时候加入堆中，维护一个无序哈希记录
            if((*iter)->getRequest().connection == "keep-alive"&&connfdHash.count((*iter)->connFd())==0)
            {
                users[(*iter)->connFd()].sockfd = (*iter)->connFd();
                users[(*iter)->connFd()].epollfd = this->epollfd;
                std::shared_ptr<Timer> timer = std::make_shared<Timer>();
				timer->user_data = &users[(*iter)->connFd()];//注意是指针
                timer->cb_func = cb_func;
                time_t cur = time( NULL );
                timer->expire = cur + 3 * TIMESLOT;
				users[(*iter)->connFd()].timer = timer.get();
				timer_lst->AddTimer(timer);
                connfdHash[(*iter)->connFd()] = 1;
                (*iter)->handle();
                delete *iter;//不关闭connfd
            }
            // 长连接复用改变时间
            else if((*iter)->getRequest().connection == "keep-alive"&&connfdHash.count((*iter)->connFd())!=0)
            {
                int sockfd = (*iter)->connFd();
                Timer* timer = users[sockfd].timer;
                if (timer)
				{
					time_t cur = time(NULL);
					timer->expire = cur + 3 * TIMESLOT;
					printf("Adjust timer once\n");
					// timer_lst.adjust_timer(timer);
				}
                (*iter)->handle();
                delete *iter;//不关闭connfd
            }
            else//短链接处理
            {
                std::cout << "Short connection" << std::endl;
                (*iter)->handle();
                removeFd((*iter)->connFd());
                delete *iter;//析构了Handler，然后删除connfd
            }
        }
        if(timeout)
        {
            std::cout<<"timehandler"<<std::endl;
            timer_handler();
            timeout = false;
        }
    }
}

void Epoll::removeFd(const int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

void Epoll::addToEpoll(const int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLET ;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
        std::cout << "Epoll::addToEpoll error: " << strerror(errno) << std::endl;
    std::cout << "----------Add to Epoll----------" << std::endl;
}
void Epoll::addToEpollTimerFd(const int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
        std::cout << "Epoll::addToEpoll error: " << strerror(errno) << std::endl;
    std::cout << "----------Timer Add to Epoll----------" << std::endl;
}