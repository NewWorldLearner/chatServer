#include "chatserver.hpp"
#include "json.hpp"
#include <iostream>
#include <functional>
#include <string>
#include <chatservice.hpp>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
// 对于muduo网络库来说，最重要的就是设置连接回调，消息回调和工作线程了
ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg): _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online" << endl;
    }
    else
    {
        cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline" << endl;
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 当buf并非json格式式，下面的解析就会产生错误
    json js = json::parse(buf);
    // 解耦网络模块的代码与业务模块的代码
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，执行相应的业务处理
    msgHandler(conn, js, time);
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    _userModel.resetState();
}
