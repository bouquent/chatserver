#ifndef GROUP_H
#define GROUP_H
#include "groupuser.hpp"
#include <string>
#include <vector>
using std::string;

class Group
{
public:
    Group(int id = -1, const string& name = " ", string desc = " ") 
            : groupid_(id)
            , groupname_(name)
            , groupdesc_(desc)
    {}

    void setGroupid(int id) { groupid_ = id; }
    void setGroupName(const string& name) { groupname_ = name; }
    void setGroupDesc(const string& desc) {groupdesc_ = desc;}
    void setGroupUsers (const std::vector<GroupUser>& groupusers) { users_ = groupusers; } 
    
    int getId() const { return groupid_; }
    const string& getGroupName() const { return groupname_; }
    const string& getGroupDesc() const { return groupdesc_; }
    std::vector<GroupUser>& getUsers()  { return users_; }
private:
    int groupid_;
    string groupname_;
    string groupdesc_;
    std::vector<GroupUser> users_; //群主里的所有成员
};

#endif