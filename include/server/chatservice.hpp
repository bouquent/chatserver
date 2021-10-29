#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include "json.hpp"
#include "redis/redis.hpp"
#include "public.hpp"
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "groupmodel.hpp"
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <mutex>
#include <map>
#include <string>
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr&, json&, Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();

    //处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理消息业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //获取了心跳包
    void heartbeat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //从redis的消息队列中获得了消息
    void handleRedisSubscribeMessage(int userid, std::string msg);

    MsgHandler getHanlder(EnMsgType msgid);

    //处理用户异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    //处理服务器异常退出
    void serverCloseException();

    //心跳测试，删除心跳测试失败的用户;
    void heartTest();
private:
    ChatService();

private:
    //数据操作user数据库的类对象
    UserModel usermodel_;
    FriendModel friendmodel_;
    OfflineMessage offlineMessage_;
    GroupModel groupmodel_;
    Redis redis_;

    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    std::unordered_map<TcpConnectionPtr, int> heartMap_;
    std::unordered_map<int, TcpConnectionPtr> userConnMap_;
    std::mutex connMutex_; //保证userConnMap_线程安全
};

#endif 