#include"chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
#include<vector>
using namespace muduo;
using namespace std;
#include<iostream>
// 获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息，以及对应的Handler 回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)}); // login 函数有3个参数
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接Redis服务器
    if(_redis.connect())
    {
        // 设置上报消息回调
        _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMessage, this, _1, _2));
    }
}

// 获取消息对应的处理器
MsgHandle ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的回调函数
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器MsgHandle， 空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR<< "msgid:" <<msgid <<"can't find Handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务 id pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);

    if(user.getId()==id && user.getPassword() == pwd)   // 为 -1的话是默认创建的User，表示没有找到对应的id
    {
        if(user.getState() == "online")
        {
            // 该用户已经登录，不允许重新登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "This account is using, input again!";
            conn->send(response.dump());
        }
        else
        {
            {
                // 作用域， 出作用于后自动释放锁
                lock_guard<mutex> lock(_connMutex);
                // 登陆成功， 记录用户连接信息
                _userConnMap.insert({id, conn});
            }

            // 登录成功， 向Redis订阅channel(id)
            _redis.subscribe(id);

            // 登录成功， 更新用户状态 0ffline->online
            user.setState("online");

            // 刷新用户状态
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["id"] = user.getId();
            response["errno"] = 0;
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(user.getId());
            if( !vec.empty() )
            {
                response["offlinemsg"] = vec;

                // 读取完离线消息后，就删除离线消息
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息, 并返回
            vector<User> uservec = _friendModel.query(id);
            if(!uservec.empty())
            {
                vector<string> vec2;
                for(User &user : uservec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询群组列表
            vector<Group> groupuserVec = _groupmodel.queryGroups(id);
            if(! groupuserVec.empty())  //有群信息
            {
                vector<string> groupV;
                for(Group &group:groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user:group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }

    }
    else
    {
        // 用户不存在， 或者用户存在密码出错，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}


// 处理注册业务 name and password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if(state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 1;
        conn->send(response.dump());       
    }
}

// 处理注销业务
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    // 用户注销，下线，在Redis中取消订阅
    _redis.unsubscribe(userid);

    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把所有用户的 online状态设置为offline
    _userModel.resetState();
}

// 客户端异常退出
void ChatService::clentCloseException(const TcpConnectionPtr &conn)
{
    // 两个步骤，1. 将map里面的数据删除，2. 将state改为offline；
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it = _userConnMap.begin(); it!=_userConnMap.end(); it++)
        {
            if(it->second == conn)
            {
                // 从map表删除用户连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 用户注销，下线，在Redis中取消订阅
    _redis.unsubscribe(user.getId());

    // 更新用户状态, -1时是无效用户
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }

}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 聊天消息  json 格式{fromid:  fromname: toid: msg:......}
    int toid = js["toid"].get<int>();

    // 保证信息安全 ,减少加锁区域
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);

        if(it != _userConnMap.end())
        {
            // toid 在线，转发消息. 服务器主动给toid用户推送消息
            it->second->send(js.dump());
            return;      
        }
    }
    // 查看toid是否在线，在别的服务器上
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return ;
    }

    // toid 不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());

}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 储存好友信息
    _friendModel.insert(userid, friendid);

}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 创建新的群组信息
    Group group(-1, name, desc);
    if(_groupmodel.createGroup(group))
    {
        // 储存群组创建人信息
        _groupmodel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    _groupmodel.addGroup(userid, groupid, "normal");
    
}

// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 聊天消息  json 格式{}
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    vector<int> useridVec = _groupmodel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int id: useridVec)
    {
        auto it = _userConnMap.find(id);

        if(it != _userConnMap.end())
        {
            // 在线，转发消息. 服务器主动给id用户推送消息
            it->second->send(js.dump());    
        }
        else
        {
            // 查询是否在线
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }  
}

// 上报消息回调
void ChatService::handlerRedisSubscribeMessage(int userid, string msg) // channel为userid
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return ;
    }

    // 储存离线消息，还在接收消息，用户刚刚下线
    _offlineMsgModel.insert(userid, msg);
}
