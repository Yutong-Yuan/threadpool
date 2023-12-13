//防止头文件重复编译
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

//用户可选的设置参数
const int _taskMaxnum=2048;
const int _threadsInitnum=4;
const int _threadsMaxnum=8;
//工作队列满时最多等待多长时间丢弃任务（单位:ms）
const int _waitQueNotFullTime=1000;  
//工作队列空时最多等待多长时间考虑关闭线程（单位:ms）
const int _waitQueNotEmptyTime=60000;


//线程池的工作模式
enum class PoolMode
{
    //线程池数量中线程数量固定
    ModeFixed,
    //线程池数量中线程数量可变（根据需求，适合小而快的任务，若耗时的任务不适合）
    ModeCached
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//定义线程类
// 线程类
class Thread
{
public:
    //线程函数对象类型
    using ThreadFunc=std::function<void(int)>;
    //线程构造
    Thread(ThreadFunc func,int threadId);
    //线程析构
    ~Thread();
    //启动线程
    void start();
private:
    ThreadFunc func_;
    int threadId_;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

    //以threadsInitnum的线程数量启动线程池，默认是固定数量线程，用户可以传入参数修改线程池模式并且指定最大线程数量
    void start(int threadsInitnum=_threadsInitnum,PoolMode poolMode=PoolMode::ModeFixed,int threadsMaxnum=_threadsMaxnum);
    //设置线程池的工作模式
    void setMode(PoolMode poolMode);

    //设置线程池最大容量
    void setThreadsMaxnum(int threadsMaxnum);
    //设置工作队列的最大容量
    void setTaskMaxnum(int taskMaxnum);
    //给线程池提交任务（修改）
    template<class Func,class ...Args>
    auto submitTask(Func func,Args... args)->std::future<decltype(func(args...))>
    {
        //获取func返回值类型
        using Rtype = decltype(func(args...));
        //将用户提交的任务用智能指针的形式存放，为的是保证任务的持久性 ps:注意packaged_task
        std::shared_ptr<std::packaged_task<Rtype()>> task(new std::packaged_task<Rtype()>(std::bind(func,args...)));
        //获取任务的返回值
        std::future<Rtype> result = task->get_future();

        std::unique_lock<std::mutex> ulock(taskQueMux_);
        //最多阻塞等待1s，若1s任务队列还是满的，则返回false
        //注意lamda表达式的捕获列表（还没听到）
        if(!notFullCv_.wait_for(ulock,std::chrono::milliseconds(_waitQueNotFullTime),[&]()->bool{return taskQue_.size()<taskMaxnum_;}))
        {
            //std::cerr是C++标准库中的一个预定义的输出流，用于向标准错误输出设备发送数据。
            std::cerr<<"task queue is full, submit task failed\n";
            //当队列满了的时候，要返回Rtype的0值
            std::unique_ptr<std::packaged_task<Rtype()>> tempTask(new std::packaged_task<Rtype()>([]()->Rtype{return Rtype();}));
            (*tempTask)();
            return tempTask->get_future();
        }

        //将task任务用lambda表达式封装成void()后加入队列
        taskQue_.emplace(std::function<void()>([task]()->void{(*task)();}));
        taskCurnum_++;
        notEmptyCv_.notify_all();

        //当目前队列中的任务比空闲线程数量的两倍还多的时候，且当前线程池设置为cached模式，且当前线程池中线程数量比最大值小的时候进行线程扩充
        for(int i=0;i<threadsMaxnum_;i++)
        {
            if(!(taskCurnum_>threadsIdlenum_*2 && poolMode_==PoolMode::ModeCached && threadsCurnum_<threadsMaxnum_))   break;
            if(threads_.count(i)==0)
            {
                threadsCurnum_++;
                threadsIdlenum_++;
                threads_.insert(std::make_pair(i,new Thread(std::bind(&ThreadPool::threadFunc,this,std::placeholders::_1),i)));
                std::cout<<"add thread: "<<i<<std::endl;
                threads_[i]->start();       
            }
        }
        return result;
    }


private:
    //线程函数
    void threadFunc(int threadId);
    //线程集合 ps:智能指针
    std::map< int,std::unique_ptr<Thread> > threads_;
    //初始线程数量
    int threadsInitnum_;
    //系统支持的最大线程数量
    int threadsMaxnum_;
    //当前线程数量
    std::atomic_int threadsCurnum_;
    //当前工作线程数量
    std::atomic_int threadsWorknum_;
    //当前空闲线程数量
    std::atomic_int threadsIdlenum_;


    //工作队列（函数对象版本）（修改）
    using Task=std::function<void()>;
    std::queue<std::function<void()>> taskQue_;

    //现在的任务数量 ps：用原子类型来表示，防止计数时出现问题
    std::atomic_int taskCurnum_;
    //工作队列的最大容量（最多可以有多少任务）
    int taskMaxnum_;

    //工作队列锁，方式存取出现问题
    std::mutex taskQueMux_;
    //工作队列不空条件变量
    std::condition_variable notEmptyCv_;
    //工作队列不满条件变量
    std::condition_variable notFullCv_;
    
    //线程池工作模式
    PoolMode poolMode_;

    //线程池是否消亡
    bool isPoolExit_;

};


#endif