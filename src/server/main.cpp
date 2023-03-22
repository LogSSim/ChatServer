#include "chatserver.hpp"
#include"chatservice.hpp"
#include<signal.h>
#include<iostream>
using namespace std;

// 处理服务器ctrl+c 结束后，重置user状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}


int main(int argc, char** argv)
{
    if(argc < 3)
    {
        cerr<<"command invalid! example :./ChatServer 127.0.0.1 7890" <<endl;
        exit(-1);
    }
    // 解析通过命令行参数 ip 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    signal(SIGINT, resetHandler);
    
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}