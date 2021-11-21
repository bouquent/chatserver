#include "redis/redis.hpp"
#include <iostream>
#include <thread>

Redis::Redis()
    : publish_context_(nullptr)
    , subcribe_context_(nullptr)
{}

Redis::~Redis()
{
    if (publish_context_ != nullptr) {
        redisFree(publish_context_);
    }
    if (subcribe_context_ != nullptr) {
        redisFree(subcribe_context_);
    }
}

//连接reids服务器
bool Redis::connect()
{
    //负责publish发布消息的上下文连接
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (nullptr == publish_context_) {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    subcribe_context_ = redisConnect("127.0.0.1", 6379);
    if (nullptr == subcribe_context_) {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    //在单独的线程中监听通道上的时间，有消息给业务层上报
    std::thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    std::cout << "connect redis-server success! " << std::endl;
    return true;
}

//向redis指定的通道发布消息
bool Redis::publish(int channel, std::string message)
{
    redisReply *reply = (redisReply*) redisCommand(publish_context_, "publish %d %s", channel, message.c_str());
    if (nullptr == reply) {
        std::cerr << "publish commman failed!" << std::endl;
        return false;
    }
    //释放动态生成的结构体
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{
      /*直接发送subscribe命令会导致当前线程阻塞*/
    if (REDIS_ERR == redisAppendCommand(subcribe_context_, "subscribe %d", channel)) {
        std::cerr << "subscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while (!done) {
        if (REDIS_ERR == redisBufferWrite(subcribe_context_, &done)) {
            std::cerr << "subscribe comman failed!" << std::endl;
            return false;
        }   
    }   
    return true;
}

//向redis指定的通道取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(subcribe_context_, "unsubscribe %d", channel)) {
        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while (!done) {
        if (REDIS_ERR == redisBufferWrite(subcribe_context_, &done)) {
            std::cerr << "unsubscribe comman failed!" << std::endl;
            return false;
        }
    }
    
    return true;    
}

//在独立的线程中接受订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(subcribe_context_, (void**) &reply)) {
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr) {
            notify_message_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }
    }
    freeReplyObject(reply);
    std::cerr << ">>>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<<<<<<<" << std::endl;
}

//设置为上层服务的回调函数
void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    notify_message_handler_ = fn;
}