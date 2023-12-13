c++11 线程池（实现较为复杂）<br>  
概览：本程序基于c++构建线程池，用到了c++11中的新特性，如thread并发库，智能指针等<br>  
环境：Ubuntu 22.04.3   g++ 11.4.0（请确保g++版本在4.8.5之上，支持c++11）<br>  
介绍：运行时添加LD_LIBRARY_PATH防止库检索失败<br>  
测试案例：<br>
1.方法一（需要事先编译动态库）<br>
（1）进入当前目录后 g++ testcase.cpp -o testcase -I./ -L./ -lthreadpool<br>
（2）添加当前目录（或者可自行编译/移动动态库到其他目录）至LD_LIBRARY_PATH环境变量<br>
（3）运行 ./testcase<br>
2.方法二（无需事先编译动态库）<br>
（1）进入当前目录后 g++ testcase.cpp threadpool.cpp -o testcase -I./<br>
（2）运行 ./testcase<br>
ps：注意函数的定义以及如何在线程池中调用请观察测试案例。<br>
