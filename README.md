c++11 线程池<br><br>
概览：本程序基于c++11构建线程池，用到了c++11中的新特性，如thread并发库，智能指针，lambda表达式，function包装器等等<br><br>
环境：Ubuntu 22.04.3   g++ 11.4.0（请确保g++版本在4.8.5之上，支持c++11）<br><br>
介绍：已经将主代码编译成动态库可供直接调用，若需重新编译则按照动态库制造规则自行编译，注意运行时添加LD_LIBRARY_PATH防止库检索失败<br><br>
测试案例：<br>
1.方法一（需要事先编译动态库）<br>
（1）进入当前目录后 g++ testcase.cpp -o testcase -std=c++11 -I./ -L./ -lthreadpool<br>
（2）添加当前目录（或者可自行编译/移动动态库到其他目录）至LD_LIBRARY_PATH环境变量<br>
（3）运行 ./testcase<br>
2.方法二（无需事先编译动态库）<br>
（1）进入当前目录后 g++ testcase.cpp threadpool.cpp -o testcase -std=c++11 -I./<br>
（2）运行 ./testcase<br>
ps：注意函数的定义以及如何在线程池中调用请观察测试案例中的步骤(1)-(3)。<br>
