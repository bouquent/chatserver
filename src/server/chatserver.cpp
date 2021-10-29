#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
#include <iostream>
#include <signal.h>

using namespace std::placeholders;
using json = nlohmann::json;

#define ALRM_TIME 180 //每三分钟一次的心跳检测


//用于心跳检测的定时任务
void alarmhandler(int signal)
{
    /*根据检测结果，关闭没有通过检测的用户的连接*/
    ChatService::instance()->heartTest();

    alarm(ALRM_TIME); //重新设置定时任务
}

ChatServer::ChatServer(EventLoop *loop,
                    const InetAddress& listenAddr,
                    const string& Argname)
    : server_(loop, listenAddr, Argname)
    , loop_(loop)
{
    //注册连接回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    //注册消息回调
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    //设置线程数量
    server_.setThreadNum(5);
}

//启动服务
void ChatServer::start()
{
    server_.start();
    signal(SIGALRM, alarmhandler);
    alarm(ALRM_TIME);
}

//上报的连接事件
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    if (!conn->connected()) {    
        conn->shutdown();
        ChatService::instance()->clientCloseException(conn);
    }
}


//上报的读写事件
void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer *buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();

    //数据的反序列化
    json js = json::parse(buf);

    //达到的目的，解耦网络模块和业务模块的代码
    //通过js[msgid]获取业务类型，然后执行相应的回调函数handler
    EnMsgType type = (EnMsgType)js["msgid"].get<int>();
    auto msgHandler = ChatService::instance()->getHanlder(type);

    //回调消息绑定好的时间处理器，来执行相应的业务处理(业务无论是什么，这里的代码都不需要有改动)
    msgHandler(conn, js, time);
}