**本项目是linux下一些简单应用和模块的例子，其中也包括像nginx+http-flv应用的编译例子，后续会随着所学，不断更新例子**
# 1.编译目录结构如下
```
app/
├── singleapp.sh                    #一个.c文件生成的应用的编译脚本
├── projectapp.sh                   #多个文件生成的应用或模块的编译脚本
├── getuserinfo.c                   #获取用户名称、uid及加目录
├── gpiosw.c                        #用户层面操作gpio按键的例子
├── greenLedPL10.c                  #使用内核自带的leds-gpio驱动模块的列子
├── hello.c                         #hello例子
├── hhello                          #hello的project例子
│   ├── app.sh                      #hello-project的编译脚本
│   └── hello.c
├── nginx                           #nginx例子
│   ├── app.sh                      #nginx的编译脚本
│   └── nginx-remove-all.txt
├── out                             #输出的应用文件，和中间的编译文件目录
│   ├── getuserinfo                 #getuserinfo的交叉编译的可执行文件
│   └── nginx                       #nginx项目的文件
├── pollsw.c                        #使用poll方式查询按键的例子
├── README.md
├── redLedPA15.c                    #使用内核自带的leds-gpio驱动模块的列子
├── selectsw.c                      #使用select方式查询按键的例子
├── tcpclient.c                     #tcp客户端例子
├── tcpserver.c                     #tcp服务器列子，分开server线程和客户端线程
├── uart13.c                        #串口3发给串口1的在不同线程的收发处理
├── uart1.c                         #串口1的收发处理
├── udpclient.c                     #udp客户端
└── udpserver.c                     #udp服务端
```
# 2.单个例子编译命令`./singleapp.sh <aaa> [bbb]`
1. aaa是必须输入的参数，代表的是在app目录下的c文件名，即aaa.c，当其不存在时，会报错
2. bbb是可选的参数，参数nfs是需要拷贝的nfs服务器外，参数clean是清除生成文件
# 3.项目例子编译命令`./projectapp.sh <aaa> [bbb]`
1. aaa是必须输入的参数，代表的是在app目录下的项目文件名，即aaa文件夹必须存在，否则报错，进而查找aaa文件夹下的app.sh文件，用于编译整个项目
2. bbb是可选的参数，除参数nfs是需要拷贝的nfs服务器外，其他参数会传给aaa下的app.sh文件执行
