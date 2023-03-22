#include"groupmodel.hpp"
#include"db.hpp"
#include<iostream>
using namespace std;
// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname, groupdesc) values('%s', '%s')", 
        group.getName().c_str(), group.getDesc().c_str());
    
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            // 获取一下插入成功的用户数据生成的主键id, 放入user类id中
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 加入群聊
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser(groupid, userid, grouprole) values(%d, %d, '%s')", groupid, userid, role.c_str());
    
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    /*
     1. 根据userid查询groupuser表中该用户所属的群信息
     2. 根据groupid查询group中所有人员userid，state，role
    */
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from AllGroup a inner join\
            GroupUser b on a.id = b.groupid where b.userid=%d", userid);
    
    vector<Group> GroupVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            // 查询userid所有群组信息
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                GroupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    
    // 查询群组用户信息
    for(Group &group : GroupVec)
    {
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from User a inner join\
            GroupUser b on b.userid = a.id where b.groupid=%d", group.getId());
        
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser groupuser;
                groupuser.setId(atoi(row[0]));
                groupuser.setName(row[1]);
                groupuser.setState(row[2]);
                groupuser.setRole(row[3]);
                group.getUsers().push_back(groupuser);
            }
            mysql_free_result(res);
        }
    }
    return GroupVec;
}

// 根据指定的groupid 查询群组用户id列表， 除user以外， 主要用户群聊业务给群组用户其他成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d", groupid, userid);
    
    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            // 查询userid所有群组信息
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;    
}