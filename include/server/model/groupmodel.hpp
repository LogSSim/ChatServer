#ifndef GROUPMODEL_HPP
#define GROUPMODEL_HPP

#include"group.hpp"

// AllGroup表的数据操作类
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群聊
    void addGroup(int userid, int groupid, string role);

    // 查询用户所在群组信息
    vector<Group> queryGroups(int userid);

    // 根据指定的groupid 查询群组用户id列表， 除user以外， 主要用户群聊业务给群组用户其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);

};


#endif