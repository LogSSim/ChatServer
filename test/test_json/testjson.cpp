#include "json.hpp"
using json = nlohmann::json;

#include<iostream>
#include<vector>
#include<map>
#include<string>
using namespace std;

// json序列化操作1
string json1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "sha bi";
    js["to"] = "s b";
    js["msg"] = "what's your name?";

    // js转字符串, 后面再通过网络发送( json数据对象-》序列化json字符串 )------------------（重要）
    string senBuf = js.dump();
    // cout<<senBuf.c_str()<<endl;
    return senBuf;
}

// 序列化2
void json2()
{
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    cout << js << endl;
}

// 序列化3
string json3()
{
    json js;

    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    cout<<js<<endl;

    // js转字符串, 后面再通过网络发送( json数据对象-》序列化json字符串 )------------------（重要）
    string senBuf = js.dump();
    cout<<senBuf.c_str()<<endl;

    return senBuf;
}

int main()
{
    string sendBuf = json3();

    // 数据反序列化， json字符串-》反序列化 数据对象 (看做容器， 方便访问)
    json jsbuf = json::parse(sendBuf);
    // cout<<jsbuf["msg_type"]<<endl;
    // cout<<jsbuf["from"]<<endl;
    // cout<<jsbuf["to"]<<endl;
    // cout<<jsbuf["msg"]<<endl;

    vector<int> vec = jsbuf["list"];
    for(auto& p:vec)
    {
        cout<<p<<" ";
    }
    cout<<endl;
    map<int, string> m = jsbuf["path"];
    for(auto& p:m)
    {
        cout<<p.first<<" "<<p.second<<endl;
    }

    return 0;
}