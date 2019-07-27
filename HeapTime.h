#ifndef _HEAPTIME_H
#define _HEAPTIME_H
#include "Time.h"
#include <queue>
#include <memory>
#include <vector>
#include <iostream>

class HeapTimer
{
  public:
	using TimerPtr = std::shared_ptr<class Timer>;
	HeapTimer(/* args */)
	{
	}
	~HeapTimer()
	{
		auto size = que.size();
		for (int i = 0; i < size; i++)
		{
			que.pop();
		}
	}
	/*添加定时器*/
	void AddTimer(const TimerPtr &timer)
	{
		if (!timer)
			return;
		que.push(timer);
	}
	/*删除定时器（肯定是优先队列的第一个元素）想想为什么？？*/
	void DelTimer(Timer *timer)
	{
		if (!timer)
			return;
		que.pop();
	}
	/*SIGALRM 信号被触发，就在信号处理函数中执行一次tick函数，以处理到期的任务*/
	void tick()
	{
		if (que.size() == 0)
			return;
		std::cout << "time tick " << std::endl;
		time_t curr = time(NULL);
		/*从头处理到期的定时器，直到遇到一个尚未到期的定时器*/
		while (!que.empty())
		{
			auto tt = que.top();
			if (curr < tt->expire)
			{
				break;
			}
			//堆顶元素时间到期　
			else
			{
				tt->cb_func(tt->user_data);
				/*执行完就把他从队列中删除*/
				que.pop();
			}
		}
	}
	time_t TopTime()
	{
		if (que.size() == 0)
			return 5;
		auto curr = time(NULL);
		return que.top()->expire - curr;
	}
	int size()
	{
		return que.size();
	}
  private:
	/* data */
	std::priority_queue<TimerPtr, std::vector<TimerPtr>, std::greater<TimerPtr>> que;
};

#endif