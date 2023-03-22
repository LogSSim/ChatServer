#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<unordered_map>
#include<functional>
#include<muduo/net/TcpConnection.h>
#include<mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
using json = nlohmann::json;
#include"usermodel.hpp"
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"

// 表示处理消息的事件回调方法类型
using MsgHandle = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 聊天服务器 业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群聊业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 登出业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandle getHandler(int msgid);

    // 客户端异常退出
    void clentCloseException(const TcpConnectionPtr &conn);

    // 服务器异常，业务重置方法
    void reset();

    // 上报消息回调
    void handlerRedisSubscribeMessage(int userid, string msg);

private:
    // 用单例设计， 构造函数私有化
    ChatService();

    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandle> _msgHandlerMap;

    // 存储在线用户的通信连接, 需要注意线程安全问题
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap线程安全问题
    mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;

    // 离线消息对象
    OfflineMsgModel _offlineMsgModel;
    
    // 存储好友信息
    FriendModel _friendModel;

    // 群组相关
    GroupModel _groupmodel;

    // redis操作对象
    Redis _redis;

};

#endif