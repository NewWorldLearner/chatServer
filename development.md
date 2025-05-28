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

# 第3章 服务器

## 网络模块

我们主要使用muduo提供的网络库来编写一个聊天服务器。

我们构建了一个回声服务器。

为了使外部机器能够访问我们的服务器，需要关闭防火墙，并进入云服务器的实例控制台，添加防火墙的放通规则。
测试外部机器是能够成功访问我们的回声服务器的，但是不知道为什么显示出来的IP地址不符合预期。
关闭防火墙：
telnet 43.139.152.229 6000

## 业务模块

我们不希望网络模块与业务模块存在强耦合，因此我们通过回调函数，将网络模块的代码与业务模块的代码分隔开来。

## 安装mysql

我使用的是腾讯的云服务器，上面似乎默认安装了mysql，但是我不知道mysql的初始密码，因此我需要先把原来的mysql卸载掉。
yum remove mysql mysql-server mysql-libs compat-mysql51
rm -rf /var/lib/mysq
rm /etc/my.cnf
find / -name mysql
然后删掉找出来的mysql文件

使用yum install命令的时候，并不能直接安装mysql，似乎与/etc/yum.conf中的配置有关，该配置中排除了mysql等软件的安装。
我使用的是如下的命令来安装mysql，默认没有初始密码。

dnf -y install mysql-server

systemctl start mysqld

mysql -uroot -p

-- 修改密码
ALTER USER 'root'@'localhost' IDENTIFIED BY 'NewPassword123!';

-- 刷新权限
FLUSH PRIVILEGES;

-- 退出
exit;

## 实现数据库代码封装

我们希望将业务模块与数据模块拆分开来，使得业务某块处理的都是数据对象，而非赤裸裸的实际数据。

我们使用的数据库是mysql，通过c++代码来操作数据库时，需要使用到mysql提供的三方库。

在查找mysql的开发库的时候，我替换了腾讯云服务器的源，但还是找不到mysql的开发包，于是使用命令来查看源中的mysql相关开发包
sudo yum search mysql*devel

查找到如下开发包并安装
sudo yum install mysql-devel.x86_64

经过以上，我们就可以成功使用mysql的开发包了。

## 数据库表设计

创建user表，注意要先创建数据库chat，并切换到该数据库中创建相应的表

CREATE TABLE user (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '用户ID',
    name VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名',
    password VARCHAR(50) NOT NULL COMMENT '用户密码',
    state ENUM('online', 'offline') DEFAULT 'offline' NOT NULL COMMENT '登录状态'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户表';

## 实现用户注册业务

所谓的用户注册，就是在数据库的user表中添加用户的信息，最重要的就是用户名和密码。

## 实现用户登录业务

用户登录就是当客户端传过来用户的id和密码的时候，查询数据库并进行比对。
用户登录可能会成功，也可能会失败，需要返回相应的信息给客户端。

## 记录用户的连接信息

当发送消息给在线好友时，消息首先是从客户端发送给服务器，服务器再将消息转发给在线好友。
那么，服务器是如何知道好友使用的是哪个连接呢？因此，服务器需要记录好友id对应的连接信息，才能将消息转发给正确的接收者。

