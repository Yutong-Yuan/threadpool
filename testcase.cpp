#include<iostream>
#include <vector>
#include"threadpool.h"

using namespace std;

//用户自定义函数
int func1(int a,int b,int c)
{
    c=a+b+c;
    return c;
}

int main()
{
    {
        ThreadPool threadPool;                                      //(1)
        cout<<"press enter to start threadpool:";
        cin.get();
        threadPool.start(6,PoolMode::ModeCached,12);                //(2)
        future<int> res[100];
        for(int i=0;i<100;i++)
        {
            function<int(int,int,int)> func=func1;
            res[i]=threadPool.submitTask(func,10,20,30);            //(3)
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        for(int i=0;i<100;i++)
        {
            cout<<i<<" : ";
            cout<<res[i].get()<<endl;
        }
        cout<<"press enter to stop threadpool:";
        cin.get();
        // threadPool.stop();
        // cout<<"press enter to exit testcase:";
        // cin.get();
    }
    cout<<"press enter to exit testcase:";
    cin.get();
    return 0;
}