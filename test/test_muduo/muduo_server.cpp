/*
    muduo网络库给用户提供了两个主要的类
    TcpServer : 用于编写服务器程序
    TcpServer ：用于编写服务器程序

    epoll + 线程池
    好处： 能够把网络I/O的代码和  业务代码区分开
                                用户的连接与断开    用户的可读写事件

*/
#include<muduo/net/TcpServer.h>
#include<muduo/net/TcpClient.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<string>
#include<functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*
    基于muduo 网络库开发服务器程序  主要在于onConnection 与 onMessage
    1. 组合TcpServer 对象
    2. 创建eventloop 事件循环指针
    3. 明确TcpServer 构造函数 需要什么参数， 输出ChatServer 的构造函数
    4. 在当前服务器类的构造函数中，注册处理连接的回调函数、以及处理读写事件的回调函数
    5. 设置合适的服务端线程数量， muduo库自己会划分I/O线程 与worker线程
*/
class ChatServer
{
    public:
        ChatServer(EventLoop* loop,             // 事件循环
            const InetAddress& listenAddr,      // IP+Port
            const string& nameArg)              //服务器名字
            : _server(loop, listenAddr, nameArg), _loop(loop)
        {
            // 给服务器注册用户连接的创建和断开回调, 当有用户创建或者断开时转到onConnection 处理
            _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));  // 绑定器

            // 给服务器注册用户读写事件的回调
            _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));    // 有三个参数

            // 设置服务器的线程数量：1个I/O线程（用户连接断开）， 3个worker线程（处理连接读写）
            _server.setThreadNum(4);

        }

        //开启事件循环
        void start()
        {
            _server.start();
        }

    private:
        // 专门处理用户的连接创建与断开 epoll listenfd accept， 此函数有两个参数， 还有一个this指针，因此需要绑定器绑定
        void onConnection(const TcpConnectionPtr& conn)
        {

            if(conn->connected())   // toipport将ip和port都返回
            {
                cout<< conn->peerAddress().toIpPort()<<" -> "<<
                    conn->localAddress().toIpPort()<<" state:online "<<endl;                
            }
            else
            {
                cout<< conn->peerAddress().toIpPort()<<" -> "<<
                    conn->localAddress().toIpPort()<<" state:offline "<<endl; 

                conn->shutdown(); // 相当于close(fd);
                // _loop->quit();   断开服务器                
            }
        }

        // 专门处理用户的读写事件
        void onMessage(const TcpConnectionPtr& conn,    // 连接
                            Buffer* buffer,             // 缓冲区类实例
                            Timestamp time)      // 接收到数据的时间信息
        {
            string buf = buffer->retrieveAllAsString(); // 将缓存区的数据放到字符串当中
            cout<<" recv data: "<< buf<<" time "<< time.toString()<< endl;
            conn->send(buf);    //发送，收到什么发什么
        }
        
        TcpServer _server;  // 1
        EventLoop *_loop;   // 2 epoll
};

int main()
{
    EventLoop loop;    //epoll
    InetAddress addr("127.0.0.1", 7890);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();   // listenfd epoll_ctl=>epoll， 将监听fd添加到epoll上
    loop.loop();    // epoll_wait 以阻塞的方式等待新用户连接， 已连接用户的读写事件等

    return 0;
}