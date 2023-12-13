#include<iostream>
#include <vector>
#include"threadpool.h"

using namespace std;

//自己定义Mytask基类，继承Task，重写run方法以实现线程函数
class MyTask:public Task
{
public:
    MyTask(int begin,int end):begin_(begin),end_(end){}
    Any run()
    {
        long sum=0;
        for(int i=begin_;i<=end_;i++)    
        {
            sum+=i;
        }
        return sum;
    }
private:
    int begin_,end_;
};

int main()
{     
    {
        ThreadPool threadPool;
        cout<<"press enter to start threadpool:";
        cin.get();
        threadPool.start(6,PoolMode::ModeCached,12);
        Result res1=threadPool.submitTask(make_shared<MyTask>(1,10000));
        long sum=res1.get().cast<long>();
        cout<<sum<<endl;
        cout<<"press enter to stop threadpool:";
        cin.get();
    }
    cout<<"press enter to exit testcase:";
    cin.get();
    return 0;
}