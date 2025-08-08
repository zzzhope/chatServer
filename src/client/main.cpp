#include "json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
using namespace std;
using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();
// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 在全局添加线程管理变量
static std::thread *g_readThread = nullptr;
static bool g_threadRunning = false;
static std::mutex g_threadMutex;

// 聊天客户端的实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // main线程用于接收用户的输入，负责发送数据
    for (;;)
    {
        // 每次循环创建新的socket连接
        // 创建client端的socket
        int clientfd = socket(AF_INET, SOCK_STREAM, 0);
        if (clientfd == -1)
        {
            cerr << "socket create error" << endl;
            continue; // 继续尝试而不是退出
        }

        // 填写client需要连接的server信息ip+port
        sockaddr_in server;
        memset(&server, 0, sizeof(sockaddr_in));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr(ip);

        // client和server进行连接
        if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
        {
            cerr << "connect server error" << endl;
            close(clientfd);
            exit(-1); // 直接退出程序
        }

        // 显示首页菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "user password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv login response error" << endl;
                }
                else if (len == 0)
                {
                    cerr << "server disconnected during login" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"].get<int>() != 0) // 登录失败
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else // 登录成功
                    {
                        // 记录当前用户的id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录当前用户的好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            // 初始化好友信息
                            g_currentUserFriendList.clear();

                            vector<string> vecFriend = responsejs["friends"];
                            for (string &str : vecFriend)
                            {
                                json frdjs = json::parse(str);
                                User userFriend;
                                userFriend.setId(frdjs["id"].get<int>());
                                userFriend.setName(frdjs["name"]);
                                userFriend.setState(frdjs["state"]);
                                g_currentUserFriendList.push_back(userFriend);
                            }
                        }

                        // 记录当前用户的群组列表消息
                        if (responsejs.contains("groups"))
                        {
                            // 初始化群组信息
                            g_currentUserGroupList.clear();

                            vector<string> vecGroup = responsejs["groups"];
                            for (string &groupstr : vecGroup)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vecUser = grpjs["users"];
                                for (string &userstr : vecUser)
                                {
                                    json userjs = json::parse(userstr);
                                    GroupUser user;
                                    user.setId(userjs["id"].get<int>());
                                    user.setName(userjs["name"]);
                                    user.setState(userjs["state"]);
                                    user.setRole(userjs["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        // 显示登录用户的基本信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息  个人聊天信息或群组消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> offlineVec = responsejs["offlinemsg"];
                            for (string &offlinestr : offlineVec)
                            {
                                json offlinejs = json::parse(offlinestr);
                                // time+[id]+name+" said: "+msg
                                if (ONE_CHAT_MSG == offlinejs["msgid"].get<int>())
                                {
                                    cout << offlinejs["time"].get<string>() << " [" << offlinejs["id"] << "] " << offlinejs["name"].get<string>() << " said: " << offlinejs["msg"].get<string>() << endl;
                                }
                                else
                                {
                                    cout << "group[" << offlinejs["groupid"] << "] message: " << offlinejs["time"].get<string>() << " [" << offlinejs["id"] << "] " << offlinejs["name"].get<string>() << " said: " << offlinejs["msg"].get<string>() << endl;
                                }
                            }
                        }

                        // 登录成功，启动接收线程负责接收数据，该线程只启动一次
                        // static int readthreadnumber = 0;
                        // if (readthreadnumber == 0)
                        // {
                        //     std::thread readTask(readTaskHandler, clientfd);
                        //     readTask.detach();
                        //     readthreadnumber++;
                        // }
                        {
                            std::lock_guard<std::mutex> lock(g_threadMutex);
                            if (g_readThread != nullptr && g_threadRunning)
                            {
                                // 停止旧线程
                                g_threadRunning = false;
                                if (g_readThread->joinable())
                                    g_readThread->join();
                                delete g_readThread;
                            }
                            g_threadRunning = true;
                            g_readThread = new std::thread(readTaskHandler, clientfd);
                            g_readThread->detach();
                        }

                        // 进入聊天主菜单页面
                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;

        case 2: // register业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error: " << request << endl;
            }
            else if (len == 0)
            {
                cerr << "server disconnected during register" << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"].get<int>() != 0) // 注册失败
                    {
                        cerr << name << " is already exist, register error!" << endl;
                    }
                    else // 注册成功
                    {
                        cout << name << " register success, userid is " << responsejs["id"] << ", do not forget it!" << endl;
                    }
                }
            }
        }
        break;

        case 3: // quit业务
            close(clientfd);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }

        // 在循环结束时关闭socket
        close(clientfd);
    }
    return 0;
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << endl;
    cout << "----------------------friend list----------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// 接收线程
void readTaskHandler(int clientfd)
{
    // for (;;)
    while (g_threadRunning)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 阻塞了
        if (len == 0 || len == -1)
        {
            // 连接断开，优雅退出线程而不是终止程序
            cout << "Connection lost, receive thread exiting..." << endl;
            {
                std::lock_guard<std::mutex> lock(g_threadMutex);
                g_threadRunning = false;
                isMainMenuRunning = false; // 通知主线程退出菜单
            }
            return; // 优雅退出线程
            // close(clientfd);
            // exit(-1);
        }

        // 接受ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            // time+[id]+name+" said: "+msg
            cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == ONE_CHAT_MSG_ACK || msgtype == ADD_FRIEND_MSG_ACK)
        {
            // 处理聊天响应消息以及处理添加好友响应消息
            if (js.contains("errno") && js["errno"].get<int>() != 0)
                cout << "Error: " << js["errmsg"].get<string>() << endl;
            continue;
        }
        if (msgtype == GROUP_CHAT_MSG)
        {
            cout << "group[" << js["groupid"] << "] message: " << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == CHANGE_PWD_MSG_ACK)
        {
            // 处理修改密码响应消息
            if (js["errno"].get<int>() == 0)
                cout << "successfully change the password!" << endl;
            else
                cout << "failed! " << js["errmsg"].get<string>() << endl;
            continue;
        }
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday, (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "changepwd" command handler
void changepwd(int, string);
// "quit" command handler
void logout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"changepwd", "群聊，格式changepwd:oldpassword:newpassword"},
    {"logout", "注销，格式logout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"changepwd", changepwd},
    {"logout", logout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，新功能不需要修改该函数
        if (idx == -1)
        {
            it->second(clientfd, ""); // 没有参数的命令
        }
        else
        {
            it->second(clientfd, commandbuf.substr(idx + 1)); // 调用命令处理的方法
        }
    }
}

// "help" command handler
void help(int, string)
{
    cout << "show command list>>>>" << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// "chat" command handler
void chat(int clientfd, string str)
{
    // friend:message
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

// "creategroup" command handler  groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}

// "groupchat" command handler  groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}

// "changepwd" command handler  groupid:message
void changepwd(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "changepwd command invalid!" << endl;
        return;
    }
    string oldpassword = str.substr(0, idx).c_str();
    string newpassword = str.substr(idx + 1, str.size() - idx);

    if (newpassword.empty())
    {
        cerr << "newpassword can't be empty!" << endl;
        return;
    }

    json js;
    js["msgid"] = CHANGE_PWD_MSG;
    js["id"] = g_currentUser.getId();
    js["oldpassword"] = oldpassword;
    js["newpassword"] = newpassword;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}

// "logout" command handler
void logout(int clientfd, string)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send logout msg error -> " << buffer << endl;
    }
    // else
    //     isMainMenuRunning = false;

    // 停止接收线程
    {
        std::lock_guard<std::mutex> lock(g_threadMutex);
        g_threadRunning = false;
        if (g_readThread != nullptr && g_readThread->joinable())
        {
            g_readThread->join(); // 等待线程结束
            delete g_readThread;
            g_readThread = nullptr;
        }
    }

    // 清理全局状态
    g_currentUser = User();
    g_currentUserFriendList.clear();
    g_currentUserGroupList.clear();

    isMainMenuRunning = false;
    cout << "Logout successful!" << endl;
}
