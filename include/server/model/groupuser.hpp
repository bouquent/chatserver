#ifndef GROUPUSER_H
#define GROUPUSER_H
#include <string>
#include <user.hpp>

using std::string;

class GroupUser : public User
{
public:
    void setRole(const string& role) { role_ = role; }
    const string& getRole() const { return role_; }
private:
    string role_; //该成员在这个组里面扮演的身份
};

#endif