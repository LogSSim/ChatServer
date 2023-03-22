#ifndef REDIS_HPP
#define REDIS_HPP

#include<hiredis/hiredis.h>
#include<thread>
#include<functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接Redis服务器
    bool connect();

    // 向Redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向Redis指定通道订阅消息
    bool subscribe(int channel);

    // 向Redis指定的通道unsubscribe 取消订阅消息 // 用户下线
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的信息
    void observer_channel_message();

    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);

private:
    // hiredis同步上下文对象，负责publish消息
    redisContext *_publish_context;

    // hiRedis同步上下文对象，负责subscribe消息
    redisContext *_subscribe_context;

    // 回调操作， 收到订阅消息，上报给service 层
    function<void(int, string)> _notify_message_handler;
};



#endif