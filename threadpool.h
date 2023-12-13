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


//Task类的前置声明
class Task;

//线程池的工作模式
enum class PoolMode
{
    //线程池数量中线程数量固定
    ModeFixed,
    //线程池数量中线程数量可变（根据需求，适合小而快的任务，若耗时的任务不适合）
    ModeCached
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//可以代表任意类型返回值的类型类
class Any
{
public:
    Any()=default;

    template <class T>
    //用子类对象初始化基类指针（通过函数返回值收）
    Any(T data2):base_(new Derive(data2)){};

    template<typename T>
    //用户获取data的途径
	T cast()
	{
        //将基类指针强行转化为对应类型的子类指针（在这种条件下一定是安全的，因为基类指针就是子类对象初始化的）
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
        //如果用户指定类型错误则pd为空指针
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data;
	}

    //基类
    class Base
    {
    public:
        virtual ~Base()=default;
    };

    //用类模版定义子类，其中子类的data就是函数的返回值
    template <class T>
    class Derive:public Base
    {
    public:
        Derive(T data2):data(data2){}
        T data;
    };
private:
    //定义基类指针，之后用于指向子类的对象
    std::unique_ptr<Base> base_;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//信号量类
class Semaphore
{
public:
    Semaphore(int limit=1,int cur=0):limit_(limit),cur_(cur){}
    void wait()
    {
        std::unique_lock<std::mutex> ulock(mux_);
        //当没有信号可用的时候阻塞
        cv_.wait(ulock,[&]()->bool{return cur_>0;});
        cur_--;
    }

    void post()
    {
        std::unique_lock<std::mutex> ulock(mux_);
        //信号量++，最大为limit_
        cur_=(cur_+1)>limit_?limit_:(cur_+1);
        cv_.notify_all();
    }
private:
    //信号量最大为多少
    int limit_;
    //当前还有多少信号可用
    int cur_;
    std::mutex mux_;
    std::condition_variable cv_;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//实现接收提交到线程池的task任务执行完成后的返回值类型Result
class Result
{
public:
    Result(std::shared_ptr<Task> task,bool isValid);
    //函数运行完毕之后设置返回值到any_中
    void set(Any any);
    //用户获取any_
    Any get();
private:
    Any any_;
    Semaphore sem_;
    std::shared_ptr<Task> task_;
    bool isValid_;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//定义线程类
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
class Task
{
public:
    Task();
    ~Task()=default;
    //将Result与task绑定
    void setRes(Result * res);
    //表示任务如何运行，纯虚函数需要子类实现，而且子类运行函数可以有不同的返回值，如何不用Any类型（c++17）来表示返回值
    virtual Any run()=0;
    //运行函数其中包含run的多态调用以及如何返回运行结果给result
    void exec();
private:
    //与task绑定的result
    Result *res_;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

    //以threadsInitnum的线程数量启动线程池，默认是固定数量线程，用户可以传入参数修改线程池模式并且指定最大线程数量
    void start(int threadsInitnum=4,PoolMode poolMode=PoolMode::ModeFixed,int threadsMaxnum=8);
    //设置线程池的工作模式
    void setMode(PoolMode poolMode);

    //设置线程池最大容量
    void setThreadsMaxnum(int threadsMaxnum);
    //设置工作队列的最大容量
    void setTaskMaxnum(int taskMaxnum);
    //给线程池提交任务
    Result submitTask(std::shared_ptr<Task> sp);

    // //设置线程池初始容量
    // void setThreadsInitnum(const int &threadsInitnum);


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

    //工作队列  ps：用智能指针表示，防止用户提交完任务之后将任务释放导致指针成野指针
    std::queue< std::shared_ptr<Task> > taskQue_;
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
    //线程池是否消亡条件变量
    std::condition_variable isExitCv_;

};


#endif