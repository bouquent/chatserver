#ifndef REDIS_H
#define REDIS_H
#include <hiredis/hiredis.h>
#include <functional>
#include <string>

class Redis
{
public:
    Redis();
    ~Redis();

    //连接reids服务器
    bool connect();

    //向redis指定的通道发布消息
    bool publish(int channel, std::string message);

    //向redis指定的通道订阅消息
    bool subscribe(int channel);

    //向redis指定的通道取消订阅消息
    bool unsubscribe(int channel);

    //在独立的线程中接受订阅通道中的消息
    void observer_channel_message();

    //设置为上层服务的回调函数
    void init_notify_handler(std::function<void(int, std::string)> fn);
private:
    //hiredis同步上下文对象，负责publish消息
    redisContext *publish_context_;

    //hiredis同步上下文对象，负责subscribe消息
    redisContext *subcribe_context_;

    //回调操作，收到订阅的消息，给service层上报
    std::function<void(int, std::string)> notify_message_handler_;
};

#endif 