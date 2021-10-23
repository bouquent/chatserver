#include <iostream>
#include <signal.h>
#include "chatserver.hpp"
#include "chatservice.hpp"

void resethandler(int sig)
{
    ChatService::instance()->serverCloseException();
    exit(0);
}

int main(int argc, char **argv)
{
    using namespace std;
    if (argc < 2) {
        cout << "input invaild port!" << endl;
        exit(1);
    }

    signal(SIGINT, resethandler);
    EventLoop loop;
    InetAddress listenAddr("127.0.0.1", atoi(argv[1]));
    ChatServer server(&loop, listenAddr, "czz");

    server.start();

    //开启mainloop时间循环
    loop.loop();
    return  0;
}