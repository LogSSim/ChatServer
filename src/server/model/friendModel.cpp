#include"friendmodel.hpp"
#include"db.hpp"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend (userid, friendid) values(%d, %d)", userid, friendid);
    
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表 friendid
vector<User> FriendModel::query(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from User a inner join Friend b on b.friendid = a.id where b.userid = %d", userid);

    MySQL mysql;
    vector<User> vec;

    if(mysql.connect())
    {
        // 查询
        MYSQL_RES *res = mysql.query(sql);

        if(res != nullptr)
        {
            // 把用friend对象放入vec
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }            
            mysql_free_result(res);           
        }
    }
    return vec;
}