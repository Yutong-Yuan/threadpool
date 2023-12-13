#include "threadpool.h"
#include <thread>
#include <iostream>
#include <mutex>
#include <map>

//ThreadPool类方法
//构造函数
ThreadPool::ThreadPool():
    threadsCurnum_(0),
    taskCurnum_(0),
    taskMaxnum_(_taskMaxnum),
    isPoolExit_(false)
    {}

//析构函数
ThreadPool::~ThreadPool(){
    //等待任务全部完成
    while (taskCurnum_!=0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    //线程池结束标志
    isPoolExit_=true;
    {
        std::unique_lock<std::mutex> ulock(taskQueMux_);
        ulock.unlock();
        //通知那些等待在没有任务的线程让它们起来
        notEmptyCv_.notify_all();
    }   
    //等待所有线程都正常退出
    while (threadsCurnum_!=0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout<<"ThreadsPool is closed\n";
}

//启动线程池
void ThreadPool::start(int threadsInitnum,PoolMode poolMode,int threadsMaxnum)
{
    //线程池工作模式
    poolMode_=poolMode;
    //线程初始数量（也是线程池最少线程的数量）
    threadsInitnum_=threadsInitnum;
    //线程池中线程数量（工作+空闲）
    threadsCurnum_=threadsInitnum;
    //线程池中最大线程数量（仅在cached模式下设置有效）
    threadsMaxnum_=threadsMaxnum;
    //现在空闲线程的数量
    threadsIdlenum_=threadsInitnum;
    //现在工作线程的数量
    threadsWorknum_=0;
    
    for(int i=0;i<threadsInitnum;i++)
    {   
        //也可以直接隐式类型转换，加入线程
        threads_.insert(std::make_pair(i,new Thread(std::bind(&ThreadPool::threadFunc,this,std::placeholders::_1),i)));
    }
    for(int i=0;i<threadsInitnum;i++)
    {
        threads_[i]->start();
    }
}

//设置线程池工作模式
void ThreadPool::setMode(PoolMode poolMode)
{
    poolMode_=poolMode;
}

//设置线程池最大容量
void ThreadPool::setThreadsMaxnum(int threadsMaxnum)
{
    threadsMaxnum_=threadsMaxnum;
}

//设置工作队列的最大容量
void ThreadPool::setTaskMaxnum(int taskMaxnum)
{
    taskMaxnum_=taskMaxnum;
}

//线程函数，即线程池中的线程到底是做什么的   ps：生产者消费者模型
void ThreadPool::threadFunc(int threadId)
{
    //每个线程完成的工作
    while (1)
    {
        //std::cout<<std::this_thread::get_id()<<std::endl;
        Task task;
        {
            std::unique_lock<std::mutex> ulock(taskQueMux_);
            //notEmpty_.wait(ulock,[&]()->bool{return !taskQue_.empty();});
            //当一个线程等待60s还是没有任务，则可以考虑回收他或者是线程池要退出了要回收所有线程
            if(!notEmptyCv_.wait_for(ulock,std::chrono::milliseconds(_waitQueNotEmptyTime),[&]()->bool{return (!taskQue_.empty())||isPoolExit_;}))
            {
                //当线程池没有关闭但是本线程等待了60s没有任务的时候，若当前线程数量较多则本线程离开
                if(threadsCurnum_>threadsInitnum_ && poolMode_==PoolMode::ModeCached)
                {
                    threadsCurnum_--;
                    threadsIdlenum_--;
                    threads_.erase(threadId);
                    std::cout<<"self del thread: "<<threadId<<std::endl;
                    return;
                }
                else continue;
            }

            //当线程池关闭的时候本线程（所有线程）都离开
            if(isPoolExit_)
            {
                threadsCurnum_--;
                std::cout<<"forced del thread: "<<threadId<<std::endl;
                return;
            }

            //正常取任务
            task=taskQue_.front();
            taskQue_.pop();
            taskCurnum_--;
            threadsWorknum_++;
            threadsIdlenum_--;
            notFullCv_.notify_all();
        }
        //执行任务
        task();
        threadsWorknum_--;
        threadsIdlenum_++;
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Thread类方法
//构造函数
Thread::Thread(ThreadFunc func,int threadId):func_(func),threadId_(threadId)
{}
//析构函数
Thread::~Thread(){}
//启动线程
void Thread::start()
{
    std::thread th(func_,threadId_);
    //设置分离线程防止局部变量t出作用域被析构
    th.detach();
}

