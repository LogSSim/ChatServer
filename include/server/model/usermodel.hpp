#ifndef USERMODEL_HPP
#define USERMODEL_HPP

#include"user.hpp"

// user表的数据操作类
class UserModel
{
public:
    // user 表的增加操作
    bool insert(User &user);

    // 根据用户号码查询用户信息
    User query(int id);

    // 更新用户的状态
    bool updateState(User user);

    // 重置用户状态信息 online->offline
    void resetState();

private:

};

#endif