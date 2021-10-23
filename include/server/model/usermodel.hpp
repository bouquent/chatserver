#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"

//User表的操作类
class UserModel
{
public:
    //给User表插入数据
    bool insert(User& user);

    //根据id找出对应的user
    User query(int id);

    //更新用户状态信息
    bool updateState(User& user);

    //重置用户状态
    void resetState();
private:
};

#endif