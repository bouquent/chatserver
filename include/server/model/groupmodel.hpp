#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include "group.hpp"
#include "groupuser.hpp"
#include <string>
#include <vector>
using std::string;
using std::vector;

class GroupModel
{
public:
    //创建群组
    bool createGroup(Group &group);

    //加入群组
    void addGroup(int userid, int groupid, string role);

    //查询用户的群组信息
    vector<Group> queryGroups(int userid);

    //根据指定的groupid查找群组用户id列表(除自己)
    vector<int> queryGroups(int userid, int groupid);
private:
};


#endif 