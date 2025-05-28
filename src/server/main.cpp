#include "chatserver.hpp"
#include <iostream>
#include "chatservice.hpp"
#include <signal.h>

using namespace std;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main()
{
    signal(SIGINT, resetHandler);
    ChatService::instance()->reset();

    EventLoop loop;
    // 如果要想使外部机器能够访问我们的服务器，需要使用0.0.0.0来初始化IP
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    cout << "ChatServer start!" << endl;
    server.start();

    loop.loop();
    return 0;
}
