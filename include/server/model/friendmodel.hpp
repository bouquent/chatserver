#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include <vector>
using std::vector;

//维护好友关系的操作接口
class FriendModel
{
public:
    //添加好友关系
    void insert(int userid, int friendid);

    //返回好友用户列表
    vector<User> query(int userid);
private:
};

#endif