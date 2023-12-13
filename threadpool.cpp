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

//启动线程池  (绑定器未读懂，还没听到)
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
        // auto ptr=std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc,this));
        // threads_.emplace_back(std::move(ptr));
        
        //也可以直接隐式类型转换
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

//给线程池提交任务  ps：生产者消费者模型
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    std::unique_lock<std::mutex> ulock(taskQueMux_);
    //最多阻塞等待1s，若1s任务队列还是满的，则返回false
    //注意lamda表达式的捕获列表（还没听到）
    if(!notFullCv_.wait_for(ulock,std::chrono::milliseconds(_waitQueNotFullTime),[&]()->bool{return taskQue_.size()<taskMaxnum_;}))
    {
        //std::cerr是C++标准库中的一个预定义的输出流，用于向标准错误输出设备发送数据。
        std::cerr<<"task queue is full, submit task failed\n";
        return Result(sp,false);
    }
    taskQue_.emplace(sp);
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
            //std::cout<<"threadCurnum: "<<threadsCurnum_<<std::endl;
            //std::cout<<"threadIdlenum: "<<threadsIdlenum_<<std::endl;
            std::cout<<"add thread: "<<i<<std::endl;
            threads_[i]->start();       
        }
    }

    //在提交任务的时候就将任务句柄与返回的Result绑定
    return Result(sp,true);
}

//线程函数，即线程池中的线程到底是做什么的   ps：生产者消费者模型
void ThreadPool::threadFunc(int threadId)
{
    //每个线程完成的工作
    while (1)
    {
        //std::cout<<std::this_thread::get_id()<<std::endl;
        std::shared_ptr<Task> task;
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
        task->exec();
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
    //目前存在问题就是多线程的时出现的线程号相同的情况。
    std::thread th(func_,threadId_);
    //设置分离线程防止局部变量t出作用域被析构
    th.detach();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Result类方法
Result::Result(std::shared_ptr<Task> task,bool isValid):task_(task),isValid_(isValid)
{
    //将要获取的结果的任务与自己绑定
    task_->setRes(this);
}

void Result::set(Any any)
{
    //获取函数返回结果
    any_=std::move(any);
    sem_.post();
}

Any Result::get()
{
    if(!isValid_)   return "";
    //任务如果没有执行完，这里会阻塞用户线程
    sem_.wait();
    //将结果返回给用户
    return std::move(any_);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Task类方法
Task::Task():res_(nullptr)
{}

void Task::setRes(Result * res)
{
    //绑定
    if(res!=nullptr)    res_=res;
}

void Task::exec()
{
    //run结束的返回值直接返回给相应的result让其设置结果
    res_->set(run());
}

