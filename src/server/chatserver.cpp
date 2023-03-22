#include "chatserver.hpp"
#include "json.hpp"
#include"chatservice.hpp"
#include<iostream>
#include<string>
#include<functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 聊天服务器的主类
ChatServer::ChatServer( EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
            :_server(loop, listenAddr, nameArg),_loop(loop)
{
    // 注册链接回调
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

// 回调，上报连接信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if(!conn->connected())
    {
        ChatService::instance()->clentCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                        Buffer* buffer,
                        Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf); 
    // 目的： 完全解耦网络模块的代码和业务模块的代码（而不是去Switch）
    // 通过js["msgid "]，每一个id映射一个事件处理，获取一个业务处理器handler=》conn js time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}