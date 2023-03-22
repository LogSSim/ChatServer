#include"redis.hpp"
#include<iostream>
using namespace std;

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr)
{

}


Redis::~Redis()
{
    if(_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if(_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

// 连接Redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(_publish_context == nullptr)
    {
        cerr<< "connect redis failed!" <<endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if(_subscribe_context == nullptr)
    {
        cerr<<"connect redis failed!" <<endl;
        return false;
    }

    // 单独开启一个线程，监听通道事件
    thread t([&](){
        observer_channel_message();
    });

    t.detach();

    cout<< "connect redis-server success!" <<endl;
    return true;
}

// 向Redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(reply == nullptr)
    {
        cerr<< "publish command failed!" <<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向Redis指定通道订阅消息
bool Redis::subscribe(int channel)
{
    // subscribe命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息，因此不适用redisCommand
    // 通道消息的接收专门在observer_channel_message函数中的独立线程进行
    // 只负责发送命令，不阻塞接收Redis server响应消息，否则和notifyMsg线程抢占响应资源
    if(redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr<< "subscribe command failed!" <<endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区发送完毕（done被置一）
    int done = 0;
    while(!done)
    {
        if(redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR)
        {
            cerr<< "subscribe command failed!"<<endl;
            return false;
        }
    }
    return true;
}

// 向Redis指定的通道unsubscribe 取消订阅消息 // 用户下线
bool Redis::unsubscribe(int channel)
{
    if(redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr<< "unsubscribe command failed!" <<endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区发送完毕（done被置一）
    int done = 0;
    while(!done)
    {
        if(redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR)
        {
            cerr<< "unsubscribe command failed!"<<endl;
            return false;
        }
    }
    return true;
}

// 独立线程中接收订阅通道中的信息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while(redisGetReply(this->_subscribe_context, (void **)&reply) == REDIS_OK)
    {
        // 订阅收到的消息是一个带三元素的数组，[2]元素为message， [1]为通道号
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr<< ">>>>>>> observer_channel_message quit <<<<<<<<" << endl;
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}