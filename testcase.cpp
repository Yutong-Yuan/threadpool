#include<iostream>
#include <vector>
#include"threadpool.h"

using namespace std;

int func1(int a,int b,int c)
{
    c=a+b+c;
    return c;
}

int main()
{
    {
        ThreadPool threadPool;
        threadPool.start(6,PoolMode::ModeCached,12);
        cin.get();
        future<int> res[100];
        for(int i=0;i<100;i++)
        {
            function<int(int,int,int)> func=func1;
            res[i]=threadPool.submitTask(func,10,20,30);
            this_thread::sleep_for(10ms);
        }
        for(int i=0;i<100;i++)
        {
            cout<<i<<" : ";
            cout<<res[i].get()<<endl;
        }
        cin.get();
    }
    cin.get();
    return 0;
}