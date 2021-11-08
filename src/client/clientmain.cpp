#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <strings.h>
#include <assert.h>
#include <thread>
#include <pthread.h>
#include <time.h>
#include <map>
#include <functional>

#include "json.hpp"
#include "user.hpp"
#include "group.hpp"
#include "public.hpp"

using json = nlohmann::json;
using std::vector;
using std::string;
using std::cout;
using std::cin;
using std::endl;

//记录当前登录的用户信息
User g_currentUser;
//记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表信息
vector<Group> g_currentGroupList;
//当前用户显示的基本信息
void showCurrentUserData();

//除了主线程之外的一个接受线程, 让接受和发送消息并行处理
void* readTaskHandler(void* clientfd);
void* sendHeartPacket(void* clientfd);
//获取系统时间
string getCurrentTime();
//主聊天页面
void mainMenu(int sockfd);

//判断用户是否下线
int client_offline = 0;


int main(int argc, char **argv)
{
    //获取id地址和端口号
    if (argc < 3) {
        cout << "please input invalid address and port\n";
        exit(1);
    }
    int port = atoi(argv[2]);
    const char *ip = argv[1];
    struct sockaddr_in cli_addr;
    bzero(&cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &cli_addr.sin_addr);

    //获取socket套接字并建立连接
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);
    int ret = connect(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
    assert(ret != -1);

    int stop_client = 0;
    while (!stop_client) {
        cout << "==========================" << endl;
        cout << "1. login" << endl; 
        cout << "2. register " << endl;
        cout << "3. quit " << endl;
        cout << "==========================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        if (cin.fail()) {  
            //防止输入字母导致的无限循环
            choice = 0;
            cin.clear();
            cin.sync();
        }
        cin.get(); //读取缓冲区中剩余的字符

        switch (choice) {

        case 1 :
        {
            //输入用户名和密码
            int userid = 0;
            char password[16] = {0};
            cout << "userid : ";
            cin >> userid;  
            cin.get();                    //读取缓冲区中剩余的换行符
            cout << "password : ";
            cin.getline(password, 16);   //getline会抛弃缓冲区剩余的换行符

            //发送登录信息
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = userid;
            js["password"] = password;
            string request = js.dump();
            int len = send(sockfd, request.c_str(), request.size(), 0);
            assert(len != -1);

            //接受服务器发过来的信息
            char buffer[1024] = {0};
            len = recv(sockfd, buffer, sizeof(buffer), 0);
            assert(len != -1);
            json responsejs = json::parse(buffer);

            if (0 != responsejs["errno"].get<int>()) { //返回错误号不是0，登陆失败 
                std::cerr << responsejs["errmsg"] << std::endl;
                break;
            } else {  //登陆成功
                client_offline = 0;

                g_currentUser.setId(responsejs["id"]);
                g_currentUser.setName(responsejs["name"]);

                //获取好友信息
                if (responsejs.contains("friends")) 
                {
                    vector<string> vec = responsejs["friends"];
                    for (auto & userstr : vec) {
                        json js = json::parse(userstr);
                        User user;
                        user.setId(js["id"].get<int>());
                        user.setName(js["name"].get<string>());
                        user.setState(js["state"].get<string>());
                        g_currentUserFriendList.push_back(user);
                    }
                }
              
                //获取群组信息
                if (responsejs.contains("groups"))
                {
                    vector<string> vec = responsejs["groups"];
                    for (auto & groupstr: vec) {
                        json groupjs = json::parse(groupstr);
                        Group group;
                        group.setGroupid(groupjs["id"].get<int>());
                        group.setGroupName(groupjs["groupname"].get<string>());
                        group.setGroupDesc(groupjs["groupdesc"].get<string>());
                       
                        vector<string> users = groupjs["users"];
                        for (auto & userstr : users) {
                            json userjs = json::parse(userstr);
                            GroupUser user;
                            user.setId(userjs["id"]);
                            user.setName(userjs["name"]);
                            user.setState(userjs["state"]);
                            user.setRole(userjs["role"]);
                           
                            group.getUsers().push_back(user);
                        }
                        g_currentGroupList.push_back(group);
                    }
                }
            }
            showCurrentUserData();

            if (responsejs.contains("offlinemessage")) {
                vector<string> vec = responsejs["offlinemessage"];
                for (auto &str : vec) {
                    json js = json::parse(str);
                    if (ONE_CHAT_MSG == js["msgid"].get<int>()) {   //个人聊天
                       std::cout << js["time"].get<string>() << "[" << js["id"].get<int>() << "] " << js["name"].get<string>() 
                            << " said: " << js["msg"].get<string>() << std::endl; 
                    } 
                    else if (GROUP_CHAT_MSG == js["msgid"].get<int>()) //群聊 
                    {
                        std::cout << "群消息 ["<< js["groupid"].get<int>() << "] " << js["time"].get<string>() << "[" << js["id"].get<int>() << "] "
                            << js["name"].get<string>() << " said: " << js["msg"].get<string>() << std::endl; 
                    }
                }
            }
            //  启动线程负责接收数据(主线程用于等待用户输入，接受命令)
            /*  std::thread readTask(readTaskHandler, sockfd); c++11
            *   readTask.detach();
            */
            pthread_t tid;
            int ret = pthread_create(&tid, NULL, readTaskHandler, (void*) &sockfd);
            assert(ret == 0);
            pthread_detach(tid);

            //开启一个线程，用于发送心跳包
            pthread_t tid2;
            ret = pthread_create(&tid, NULL, sendHeartPacket, (void*) &sockfd);
            assert(ret == 0);
            pthread_detach(tid2);

            //进入聊天主菜单页面
            mainMenu(sockfd);
        }
        break;
        case 2 : 
        {
            char username[16] = {0};
            char password[16] = {0};
            cout << "username : ";
            cin.getline(username, 16); //防止将\n输入到name
            cout << "password : ";
            cin.getline(password, 16);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = username;
            js["password"] = password;
            string request = js.dump();
            int len = send(sockfd, request.c_str(), request.size(), 0);
            if (len == -1) {
                std::cerr << "send reg msg error\n";
            } else {
                char buffer[1024] = {0};
                len = recv(sockfd, buffer, sizeof(buffer), 0);
                if (len == -1)
                {
                    std::cerr << "recv reg msg error\n";
                }
                else 
                {
                    json getjs = json::parse(buffer);
                    if (0 != getjs["errno"].get<int>()) 
                    {
                        std::cerr << getjs["errmsg"] << std::endl;
                    }
                    else
                    {
                        cout << username << "register success, userid is " << getjs["id"] << ", do not froget it" << endl;
                    }
                }
            }
            break;
        }

        case 3 :
            stop_client = 1;
            break;
        default :
            std::cerr << "invaild input!" << std::endl;
            break;
        }
    } 
    close(sockfd);
    return 0;
}


void showCurrentUserData()
{
    std::cout << "==========login user==========" << std::endl;
    std::cout << "current login user => id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << std::endl;
    std::cout << "----------friend List------------" << std::endl;
    if (!g_currentUserFriendList.empty()) {
            for (auto & user : g_currentUserFriendList) {
                std::cout << user.getId() << "  " << user.getName() << "  " << user.getState() << std::endl;
            }
    }

    std::cout << "----------group List-------------" << std::endl;
    if (!g_currentGroupList.empty()) {
        for (auto &group : g_currentGroupList) {
            std::cout << "group id is " << group.getId() << "  " << "group name is " << group.getGroupName() << std::endl;
            for (auto &groupuser : group.getUsers()) {
                std::cout << "   " << groupuser.getId() <<  "  " << groupuser.getName() << "  " << groupuser.getState()\
                    << "  " << groupuser.getRole() << std::endl;
            }
        }
    }
    std::cout << "================================" << std::endl;
}

//除了主线程之外的一个接受线程, 让接受和发送消息并行处理
void* readTaskHandler(void *fd)
{
    int sockfd = (*(int*)fd);

    while (!client_offline) {
        char buffer[1024] = {0};
        int len = recv(sockfd, buffer, sizeof(buffer), 0);
        if (len == -1) {
            std::cerr << "recv error" << std::endl;
        }

        json js = json::parse(buffer);
        if (ONE_CHAT_MSG == js["msgid"].get<int>()) {   //个人消息
            std::cout << js["time"].get<string>() << "[" << js["id"].get<int>() << "] " 
                    << js["name"].get<string>() << " said: " << js["msg"].get<string>() << std::endl;
            continue;
        } else if (GROUP_CHAT_MSG == js["msgid"].get<int>()) {  //群聊消息
            std::cout << "群消息 ["<< js["groupid"].get<int>() << "] " << js["time"].get<string>() << "[" << js["id"].get<int>() << "] "
                    << js["name"].get<string>() << " said: " << js["msg"].get<string>() << std::endl; 
            continue;
        } else if (LOGINOUT_MSG == js["msgid"].get<int>()) {  
            //用户注销，客户端线程结束
            client_offline = 1;
        }
    }
    return nullptr;
}

//用于发送心跳包的线程
void* sendHeartPacket(void* clientfd)
{
    int sockfd = (*(int*)clientfd);
    while (!client_offline) {
        json js;
        js["msgid"] = HEART_MSG;
        string heartPaket = js.dump();
        int ret = send(sockfd, heartPaket.c_str(), heartPaket.size(), 0);
        if (ret == -1) {
            std::cerr << "sendHeartPacket send wrong!" << std::endl;
        }
        sleep(180); //每次发送间隔为三分钟
    }

    return nullptr;
}


//获取系统时间
string getCurrentTime()
{
    char buf[128] = {0};
    time_t nowtime = time(NULL);
    tm *tm_time = localtime(&nowtime);
    snprintf(buf, 128, "%4d/%2d/%02d %02d:%02d:%02d",
            tm_time->tm_year + 1900,
            tm_time->tm_mon + 1,
            tm_time->tm_mday,
            tm_time->tm_hour,
            tm_time->tm_min,
            tm_time->tm_sec);
    return (string)buf;
}

void help(int = 0, string = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

//客户端的命令列表
std::unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天， 格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群组聊天， 格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"},
};
//对应命令的回调函数
std::unordered_map<string, std::function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout},
};


//主聊天页面
void mainMenu(int sockfd)
{
    help();
    char buffer[1024];
    while (1) {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;                 //存储最终的命令
        int idx = commandbuf.find(":");
        if (-1 == idx) {
            command = commandbuf;
        } else {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (commandHandlerMap.end() == it) {
            cout << "invaild input command!" << endl;  //没有找到对应的命令
            continue;
        }

        //调用命令响应的函数
        it->second(sockfd,  commandbuf.substr(idx + 1, commandbuf.size() - idx));
        //退出登录
        if (it->first == "loginout") {  
            break;
        }
    }
}

void help(int, string)
{
    cout << "show command list>>>>>" << endl;
    for (auto & p : commandMap) {
        cout << p.first << " : " << p.second << endl; 
    }
}

void chat(int sockfd, string s)
{
    int idx = s.find(":");
    int friendid = atoi(s.substr(0, idx).c_str());
    string message = s.substr(idx + 1, s.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message.c_str();
    js["time"] = getCurrentTime();
    string sendmsg = js.dump();

    int ret = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0);
    if (-1 == ret) {
        std::cerr << "send chat msg error->" << sendmsg << std::endl;
    }
}

void addfriend(int sockfd, string s)
{
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = atoi(s.c_str());
    string sendmsg = js.dump();
    
    int ret = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0);
    if (-1 == ret) {
        std::cerr << "send addfriend msg error->" << sendmsg << std::endl;
    } 
}

void loginout(int sockfd, string s)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string sendmsg = js.dump();

    int ret = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0);
    if (-1 == ret) {
        std::cerr << "send chat msg error->" << sendmsg << std::endl;
    }

    //将当前用户的数据清空
    g_currentUser.setId(-1);
    g_currentUser.setName(" ");
    g_currentUser.setState("offline");
    vector<User>().swap(g_currentUserFriendList);
    vector<Group>().swap(g_currentGroupList);
}

void groupchat(int sockfd, string s) 
{
    int idx = s.find(":");
    int groupid = atoi(s.substr(0, idx).c_str());
    string message = s.substr(idx + 1, s.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string sendmsg = js.dump();

    int ret = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0);
    if (-1 == ret) {
        std::cerr << "send chat msg error->" << sendmsg << std::endl;
    }
}

void addgroup(int sockfd, string s) 
{
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = atoi(s.c_str());
    string sendmsg = js.dump();

    int ret = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0);
    if (-1 == ret) {
        std::cerr << "send chat msg error->" << sendmsg << std::endl;
    }
}

void creategroup(int sockfd, string s)
{
    int idx = s.find(":");
    string groupname = s.substr(0, idx);
    string groupdesc = s.substr(idx + 1, s.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname.c_str();
    js["groupdesc"] = groupdesc.c_str();
    string sendmsg = js.dump();

    int ret = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0);
    if (-1 == ret) {
        std::cerr << "send chat msg error->" << sendmsg << std::endl;
    }
}
