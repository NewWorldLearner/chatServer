# 前言

记录一个集群聊天服务器的开发过程

# 第1章 开发环境准备

使用linux开发，centOS或者ubtun皆可。

编辑器：vscode

## 从本地传输文件到服务器

在vscode中安装插件Remote Development，然后直接将文件拖拽到服务器目录中，就可以将文件传输到服务器端了。

# 第2章 json使用

我们思考一个问题，如果数据传输没有格式的话会怎么样？
比如传输一个字符串给对方，"张三30北京"，接收方将无法区分接收到的是姓名，年龄还是地址。
如果没有数据传输格式，接收方对接收到的信息难以解析，特别是数据较为复杂的时候。因此，我们希望发送数据的时候，发送的数据具有一定的传输格式。
传输格式有多种，比如xml和json。json的使用较为简单方便，因此我们使用json作为本项目的数据传输格式。

我们使用的是一个json三方库，JSON for Modern C++。
该json库只是一个头文件，里面包含了源码，因此使用比较简单，只需要包含头文件就可以使用了。


# 第3章 muduo网络库使用

muduo网络库是依赖boost库进行开发的，因此我们先安装boost库。

一篇安装muduo网络库和boost库的博客可供参考：
https://blog.csdn.net/m0_73537205/article/details/138353805

## boost安装

直接通过yum安装，简单方便。
sudo yum install boost
sudo yum install boost-devel

安装完成后，可以在/usr/local/include/boost/目录下找到Boost的头文件。
参考的博客如下：
https://www.oryoy.com/news/centos-xi-tong-qing-song-an-zhuang-boost-ku-yi-jian-jiao-cheng-rang-ni-kuai-su-ti-sheng-bian-cheng-x.html

验证boost库如下：
```cpp
#include <iostream>
#include <boost/bind/bind.hpp>
#include <string>
using namespace std;

using namespace boost::placeholders;

class Hello
{
public:
    void say(string name)
    {
        cout << name << " say: hello world!" << endl;
    }
};

int main()
{
    Hello h;
    auto func = boost::bind(&Hello::say, &h, "zhang san");
    func();
    return 0;
}
```

## muduo安装

首先clone muduo源码
git clone https://github.com/chenshuo/muduo.git

进入muduo目录，执行
./build.sh

安装mudduo
./build.sh install

进入到构建好的build目录下的include目录
cd ../build/release-install-cpp11/include

将muduo头文件拷贝到系统头文件中
sudo mv muduo/ /usr/include/

进入到构建好的build目录下的lib目录
cd ../lib

执行如下命令
sudo mv * /usr/local/lib/

## 测试muduo的使用

一个简单的编译测试
```cpp
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <boost/bind/bind.hpp>
#include <muduo/net/EventLoop.h>

using namespace boost::placeholders;


// 使用muduo开发回显服务器
class EchoServer
{
public:
    EchoServer(muduo::net::EventLoop *loop,
               const muduo::net::InetAddress &listenAddr);

    void start();

private:
    void onConnection(const muduo::net::TcpConnectionPtr &conn);

    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buf,
                   muduo::Timestamp time);

    muduo::net::TcpServer server_;
};

EchoServer::EchoServer(muduo::net::EventLoop *loop,
                       const muduo::net::InetAddress &listenAddr)
    : server_(loop, listenAddr, "EchoServer")
{
    server_.setConnectionCallback(
        boost::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start()
{
    server_.start();
}

void EchoServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
             << conn->localAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");
}

void EchoServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,
                           muduo::Timestamp time)
{
    // 接收到所有的消息，然后回显
    muduo::string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
             << "data received at " << time.toString();
    conn->send(msg);
}

int main()
{
    LOG_INFO << "pid = " << getpid();
    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr(8888);
    EchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
}
```

编译命令如下：
g++ test_muduo.cpp -lmuduo_net -lmuduo_base