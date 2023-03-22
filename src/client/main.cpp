#include"json.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
using namespace std;
using json = nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include"user.hpp"
#include"group.hpp"
#include"public.hpp"

// 控制主菜单页面
bool isMainMenuRunning = false;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户好友列表
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组消息
vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接收线程  主线程发送线程，子线程做接收线程
void readTaskHandler(int clientfd);
// 获得系统时间（聊天信息需要添加事件信息）
string getCurrentTime();
// 主聊天界面程序
void mainMenu(int clientfd);

// 聊天客户端实现， main线程用作发送线程，主线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if(argc < 3)
    {
        cerr<<"command invalid! example :./ChatClient 127.0.0.1 7890" <<endl;
        exit(-1);
    }

    // 解析通过命令行参数 ip 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1)
    {
        cerr<< "socket create error"<<endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    // client与server连接
    if(connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr<< "connect server error."<<endl;
        close(clientfd);
        exit(-1);
    }

    // main线程负责接受用户输入以及负责发送数据
    for(; ;)
    {
        // 显示首页面菜单，登录、注册、退出
        cout<<"====================="<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"====================="<<endl;
        cout<<"choice:";
        int choice = 0;
        cin>> choice;
        cin.get();  // 读掉缓冲区残留的回车

        switch(choice)
        {
            case 1: //登录业务
            {
                int id = 0;
                char pwd[50] = {0};
                cout<<"userid: ";
                cin>>id;
                cin.get();
                cout<<"userpassword: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;

                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
                if(len == -1)
                {
                    cerr<<"send login msg error."<<endl;
                }
                else
                {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if(len == -1)
                    {
                        cerr<<"recv login response error."<<endl;
                    }
                    else
                    {
                        json responsejs = json::parse(buffer);  // 反序列化
                        if(responsejs["errno"].get<int>() != 0) // 登录失败
                        {
                            cerr<<responsejs["errmsg"]<<endl;
                        }
                        else    // 登陆成功
                        {
                            // 记录当前的用户id 和 name；
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);

                            // 记录当前好友列表信息
                            if(responsejs.contains("friends"))
                            {
                                vector<string> vec = responsejs["friends"];
                                for(string str:vec)
                                {
                                    json js = json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }
                            
                            // 记录当前群组列表
                            if(responsejs.contains("groups"))
                            {
                                vector<string> groupV = responsejs["groups"];
                                for(string &str:groupV)
                                {
                                    json grojs = json::parse(str);
                                    Group group;
                                    group.setId(grojs["id"].get<int>());
                                    group.setName(grojs["groupname"]);
                                    group.setDesc(grojs["groupdesc"]);
                                    vector<string> userV = grojs["users"];

                                    for(string &user:userV)
                                    {
                                        GroupUser guser;
                                        json userjs = json::parse(user);
                                        guser.setId(userjs["id"]);
                                        guser.setName(userjs["name"]);
                                        guser.setState(userjs["state"]);
                                        guser.setRole(userjs["role"]);
                                        group.getUsers().push_back(guser);
                                    }

                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            // 显示当前登录用户基本信息
                            showCurrentUserData();

                            // 显示当前登录账号的离线消息 群消息以及个人消息
                            if(responsejs.contains("offlinemsg"))
                            {
                                vector<string> vec = responsejs["offlinemsg"];
                                for(string &str:vec)
                                {
                                    json js = json::parse(str);
                                    // 时间 + id + name + msg
                                    int msgtype = js["msgid"].get<int>();
                                    if(msgtype == ONE_CHAT_MSG)
                                    {
                                        cout<< js["time"]<<"["<<js["id"]<<"]"<<js["name"]<<"says: "<<js["msg"]<<endl;
                                    }
                                    else
                                    {
                                        // 群消息输出 
                                        cout<<"群消息["<<js["groupid"]<<"]:"
                                        <<js["time"]<<"["<<js["id"]<<"]"<<js["name"]<<"said: "<<js["msg"]<<endl;          
                                    }
                                    
                                }
                            }

                            // 登录成功，启动接收线程负责接收数据（子线程），只启动一次
                            static int readthreadnum = 0;
                            if(readthreadnum == 0)
                            {
                                std::thread readTask(readTaskHandler, clientfd);
                                readTask.detach();  // 线程分离
                                readthreadnum++;
                            }

                            isMainMenuRunning = true;
                            // 进入聊天菜单界面
                            mainMenu(clientfd);
                        }
                    }                    
                }
            }
            break;
            case 2: // 注册业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout<<"username: ";
                cin.getline(name, 50);
                cout<<"userpassword: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();
                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);

                if(len == -1)
                {
                    cerr<< "send reg msg error."<<endl;
                    // close(clientfd);
                    // exit(-1);
                }
                else
                {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if(len == -1)
                    {
                        cerr<<"recv reg response error."<<endl;
                    }
                    else
                    {
                        json responsejs = json::parse(buffer);
                        if(responsejs["errno"].get<int>() != 0) // 注册失败
                        {
                            cerr<<name<<"is already exist, register error!"<<endl;
                        }
                        else
                        {
                            cout<< name<<"register success, userid is "<<responsejs["id"]<<", do not forget it!"<<endl;
                        }
                    }
                }
            }
            break;
            case 3: //   quit
            {
                close(clientfd);
                exit(0);
            }
            default:
                cerr<< "invalid input!"<<endl;
            break;

        }
    }


    return 0;
}

// 接收线程
void readTaskHandler(int clientfd)
{
    for(; ;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);

        if(len == -1 || len == 0)
        {
            close(clientfd);
            exit(0);
        }

        // 接受ChatServer 转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(msgtype == ONE_CHAT_MSG)
        {
            cout<< js["time"].get<string>() <<"["<< js["id"]<< "]"<<js["name"].get<string>()<<" "<< "say: "<<js["msg"].get<string>()<<endl;
            continue;
        }
        else if(msgtype == GROUP_CHAT_MSG)
        {
            // 群消息输出 
            cout<< "群消息[" << js["groupid"] << "]:" <<js["time"].get<string>() <<"["<< js["id"]<< "]"<<js["name"].get<string>()
            <<" "<< "said: "<<js["msg"].get<string>()<<endl;
            continue;            
        }
    }
}

// 一个string对应一个处理函数
void help(int fd = 0, string s = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> command_Map = {
    {"help", "显示所支持的命令, 格式help"},
    {"chat", "一对一聊天, 格式chat:friend:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"loginout", "注销, loginout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天界面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        // 找commandbuf中的:
        int idx = commandbuf.find(":");
        if(idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            cerr<<"invalid input command!"<<endl;
            continue;
        }
        // 调用相应command的回调函数， mainMenu对修改的封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx+1, commandbuf.size()-idx));// 调用commandHandlerMap的函数


    }

}

// "help" command handler
void help(int, string)
{
    cout<<"show command list>>>" <<endl;
    for(auto &p : command_Map)
    {
        cout<< p.first << ":" << p.second<<endl;
    }
    cout<<endl;
}

// "addfriend"command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len == -1)
    {
        cerr<<"send addfriend msg error ->" << buffer<< endl;
    }
}

// "chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":"); // friendid:message
    if(idx == -1)
    {
        cerr<< "chat command invalid!"<<endl;
        return ;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len == -1)
    {
        cerr<<"send chat msg error ->" <<buffer<<endl;
    }
    
}

// "create group handler" groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr<< "create group command invalid!"<<endl;
        return ;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size()-idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()), 0);
    if(len == -1)
    {
        cerr<<"send create group msg error ->" << buffer <<endl;        
    }
}

// "addgroup" Handler groupid
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["groupid"] = groupid;
    js["id"] = g_currentUser.getId();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);

    if(len == -1)
    {
        cerr<< "send addgroup msg error ->"<< buffer <<endl;
    }
}

// "groupchat" Handler groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr<< "group chat invalid!"<<endl;
        return ;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size()-idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()), 0);
    if(len == -1)
    {
        cerr<<"send group chat msg error ->" << buffer <<endl;        
    }   
}

// "loginout" Handler 
void loginout(int clientfd, string str)
{ 
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);

    if(len == -1)
    {
        cerr<< "send LOG out msg error ->"<< buffer <<endl;
    }
    else
    {
        isMainMenuRunning = false; // 返回登录页面
        g_currentUserFriendList.clear();
        g_currentUserGroupList.clear();
    }
}

// 显示当前登录用户的一些信息
void showCurrentUserData()
{
    cout<<"==============================login user=============================="<<endl;
    cout<<"current login user => id:" <<g_currentUser.getId()<<" "<<"name:"<<g_currentUser.getName()<<endl;
    cout<<"------------------------------friend list------------------------------"<<endl;
    if(!g_currentUserFriendList.empty())
    {
        for(User &user:g_currentUserFriendList)
        {
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"------------------------------group list------------------------------"<<endl;
    if(!g_currentUserGroupList.empty())
    {
        for(Group &group : g_currentUserGroupList)
        {
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user: group.getUsers())
            {
                cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<" "<<user.getRole()<<endl;
            }
        }
    }
    cout<<"============================================================="<<endl;    
}

// 获得系统时间（聊天信息需要添加事件信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);

    char data[60] = {0};
    sprintf(data, "%d-%02d-%02d %02d:%02d:%02d", 
            (int) ptm->tm_year + 1900, (int)ptm->tm_mon +1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);

    return std::string(data);   
}