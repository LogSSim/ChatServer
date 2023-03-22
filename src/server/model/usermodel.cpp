#include"usermodel.hpp"
#include"db.hpp"
#include<iostream>
using namespace std;

// user 表的增加操作
bool UserModel::insert(User &user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')", 
        user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            // 获取一下插入成功的用户数据生成的主键id, 放入user类id中
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id = %d", id);
    
    MySQL mysql;
    if(mysql.connect())
    {
        // 查询
        MYSQL_RES *res = mysql.query(sql);

        if(res != nullptr)
        {
            // 查询到了, 拿一行
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row!=nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                
                // 使用完释放指针，不然会导致内存泄漏
                mysql_free_result(res);
                return user;
            }
        }
        else
        {
            // 没查到
            return User();
        }
    }

    return false;   
}

// 更新用户的状态
bool UserModel::updateState(User user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d", user.getState().c_str(), user.getId());
    
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }

    return false;
}

// 重置用户状态信息
void UserModel::resetState()
{
    // 1. 组装sql语句
    char sql[1024] = "update User set state = 'offline' where state = 'online'";
    // sprintf(sql, "update User set state = 'offline' where state = 'online'");
  
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}