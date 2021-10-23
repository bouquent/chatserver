#ifndef USER_H
#define USER_H
#include <string>
using std::string;

class User{
public:
    User(int id = -1, const string &name = " ", const string &password = " ", const string& state = "offline")
        : id_(id)
        , name_(name)
        , password_(password)
        , state_(state)
    {}


    int getId() const {return id_;}
    const string& getName() const {return name_;}
    const string& getPassword() const {return password_;}
    const string& getState() const {return state_;}
    
    void setId(int id) {id_ = id; }
    void setName(const string& name) {name_ = name;}
    void setPassword(const string& password) {password_ = password;}
    void setState(const string& state) {state_ = state;}
private:
    int id_;
    string name_;
    string password_;
    string state_;
};

#endif