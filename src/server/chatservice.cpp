#include "chatservice.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <muduo/base/Logging.h>
using namespace std::placeholders;
using std::vector;
using std::string;

//获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

//注册消息以及对应的handler回调操作
ChatService::ChatService()
{
    //用户业务回调操作
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});

    msgHandlerMap_.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    msgHandlerMap_.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)}); 

    //群组业务回调操作 
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)}); 
    
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)}); 

    //连接redis服务器
    if (redis_.connect()) {
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

//获取msgid对应的方法
 MsgHandler ChatService::getHanlder(EnMsgType msgid)
 {
     //如果没有对应的事件回调，记录一个只打印错误日志的信息的回调函数
     //不返回对应的回调函数，可能会被外部调用导致程序终止
     if (msgHandlerMap_.end() == msgHandlerMap_.find(msgid)) {
        return [=](auto a, auto b, auto c){
            LOG_ERROR << "msgid: "  << msgid << "can't find handler!";
        };
     }
     return msgHandlerMap_[msgid];
 }

//处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();

    string pwd = js["password"].get<string>();

    User user = usermodel_.query(id);
    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    //-1防止没有查询到结果的情况
    if (user.getId() != -1 && user.getPassword() == pwd) {
        if (user.getState() == "online")     //该用户已经登陆，不允许重复登陆
        {
            response["errno"] = -2;
            response["errmsg"] = "该账号已经登陆";
            response["id"] = user.getId();
            response["name"] = user.getName();
            conn->send(response.dump());
        }
        else
        { 
            //登录成功，记录用户连接信息
            {
                std::lock_guard<std::mutex> lock(connMutex_); 
                userConnMap_.insert({user.getId(), conn});
            }
            //向redis订阅channel(id)
            redis_.subscribe(user.getId());
 
            // 更改用户状态信息和连接信息
            user.setState("online");
            usermodel_.updateState(user);

            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            //查询当前用户的离线消息
            vector<string> vec = offlineMessage_.query(user.getId());
            if (!vec.empty()) {
                response["offlinemessage"] = vec; //读完后删除该用户的所有消息
                offlineMessage_.remove(user.getId());
            }

            //查询该用户的好友信息
            vector<User> uservec = friendmodel_.query(user.getId());
            if (!uservec.empty()) {
                vector<string> vec2;
                for (auto user : uservec) {
                    json jstmp;
                    jstmp["id"] = user.getId();
                    jstmp["name"] = user.getName();
                    jstmp["state"] = user.getState();
                    vec2.push_back(jstmp.dump());
                }
                response["friends"] = vec2;
            }

            //查询用户的群组信息
            vector<Group> groupvec = groupmodel_.queryGroups(user.getId());
            if (!groupvec.empty()) {
                vector<string> groupV;
                for (auto &group : groupvec) {
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getGroupName();
                    groupjs["groupdesc"] = group.getGroupDesc();
                    vector<string> users;
                    for (auto &groupuser : group.getUsers()) {
                        json js;
                        js["id"] = groupuser.getId();
                        js["name"] = groupuser.getName();
                        js["state"] = groupuser.getState();
                        js["role"] = groupuser.getRole();
                        users.push_back(js.dump());
                    }
                    groupjs["users"] = users;
                    groupV.push_back(groupjs.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    } else {
        //登录失败
        response["errno"] = -1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"].get<string>();
    string pwd =  js["password"].get<string>();

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = usermodel_.insert(user);

    json response;  //响应报文
    if (state == true) {
        //注册成功
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    } else {
        //注册失败
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = -1;
        response["errmsg"] = "is already exist or else err, register error!";
        conn->send(response.dump());
    }
}

//发送消息业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    //获取双方用户id值
    int id = js["id"].get<int>();
    int toid = js["toid"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto iter = userConnMap_.find(toid);
        //对方用户在线，直接转发
        if (iter != userConnMap_.end()) {   
            iter->second->send(js.dump());
            return ;
        }
    }

    User user = usermodel_.query(toid);
    if (user.getState() == "online") {
        //该用户在线，但是与id用户不在同一个服务器上
        redis_.publish(toid, js.dump());
        return ;
    }

    //对方不在线，存储离线消息
    offlineMessage_.insert(toid, js.dump());
}

//添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int friendid = js["friendid"].get<int>();   
    int id = js["id"].get<int>();
    json response;
    response["msgid"] = ELSE_MSG;

    User user = usermodel_.query(friendid);
    if (user.getId() != -1) {
        //找到friendid对应的用户
        friendmodel_.insert(id, friendid);
        response["msg"] = "add friend success!";
        conn->send(response.dump());
        return ;
    }
    //没有找到friendid对应的用户
    response["error"] = -3;
    response["msgerr"] = "don't find the friendid";
    conn->send(response.dump());
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();   //创建群聊的用户
    string name = js["groupname"].get<string>();
    string desc = js["groupdesc"].get<string>();

    Group group(-1, name, desc);
    if (groupmodel_.createGroup(group)) {
        //加入创建人的信息
        groupmodel_.addGroup(userid, group.getId(), "creator");
    }
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupmodel_.addGroup(userid, groupid, "normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> uservec = groupmodel_.queryGroups(userid, groupid);

    for (auto userid : uservec) {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto iter = userConnMap_.find(userid);
        if (iter != userConnMap_.end()) {     
            //该用户在线
            iter->second->send(js.dump());
        } else {       
            User user = usermodel_.query(userid);
            if (user.getState() == "online") 
            {
                //该用户在线，但是与userid不在同一个服务器上
                redis_.publish(userid, js.dump());
            } 
            else 
            {
                //该用户不在线，存储离线消息
                offlineMessage_.insert(userid, js.dump());
            }
        }
    }
}

//注销登陆业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int findis = 0;
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        for (auto iter = userConnMap_.begin(); iter != userConnMap_.end(); iter++) {
            if (iter->second == conn) {
                userConnMap_.erase(iter);
                findis = 1;             //如果寻找到对应的用户，则将findis标记设置为1
                break;
            }
        }
    }
    if (findis) {
        //取消订阅
        redis_.unsubscribe(userid);

        User user;
        user.setId(userid);
        user.setState("offline");
        usermodel_.updateState(user);
        json js;
        js["msgid"] = LOGINOUT_MSG;
        conn->send(js.dump());
    }
}


//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    int id = user.getId();
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        for (auto iter = userConnMap_.begin(); iter != userConnMap_.end(); iter++) {
            if (iter->second == conn) {
                user.setId(iter->first);
                userConnMap_.erase(iter);     //删除连接信息
                break;
            }
        }
    }

    //取消订阅
    redis_.unsubscribe(user.getId());

    //更改用户状态信息
    if (user.getId() != -1) {
        user.setState("offline");
        usermodel_.updateState(user);
    }
}

//处理服务器异常退出
void ChatService::serverCloseException() 
{
    //把所有online用户设置成offline
    usermodel_.resetState();
}

//从redis的消息队列中获取订阅消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    std::lock_guard<std::mutex> lock(connMutex_);
    auto it = userConnMap_.find(userid);
    if (it != userConnMap_.end()) {
        it->second->send(msg);
        return ;
    }

    //该用户下线了，存储离线消息
    offlineMessage_.insert(userid, msg);
    
}