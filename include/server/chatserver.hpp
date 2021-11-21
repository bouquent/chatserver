#ifndef CHATSERVER_H
#define CHATSERVER_H

#include "public.hpp"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    //初始化聊天服务器对象
    ChatServer(EventLoop *loop, 
            const InetAddress &listenAddr, 
            const string& Argname);
            
    ~ChatServer() = default;

    //启动服务
    void start();

private:
    //上报连接信息的回调函数
    void onConnection(const TcpConnectionPtr &conn);

    //上报读写信息的回调函数
    void onMessage(const TcpConnectionPtr &conn, 
                Buffer *buffer, 
                Timestamp time);
private:
    TcpServer server_;  //组合muduo库，实现服务器功能的类对象
    EventLoop *loop_;   //指向mainloop事件循环对象的指针
};

#endif