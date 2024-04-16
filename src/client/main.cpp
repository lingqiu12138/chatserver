#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <unordered_map>

using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "user.hpp"
#include "group.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime();
// 主聊天页面程序
void mainManu(int);
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << std::endl;
        exit(-1);
    }
    /*
    std::cerr是非缓冲的，这意味着它的输出会立即显示在终端或控制台上，而不会等待缓冲区刷新。
    这种非缓冲的特性在错误和异常处理中非常重要，因为它确保错误消息尽快显示，从而帮助程序员更快地定位和解决问题。
    */

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]); // uint16_t表示范围在0到65535之间的无符号整数

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    /*
    在socket(AF_INET, SOCK_STREAM, 0)函数调用中，每个参数的含义如下：
    AF_INET是地址族（Address Family）参数，用于指定套接字所使用的协议族或地址类型。
    AF_INET 表示使用 IPv4 地址族，它允许套接字使用 IPv4 地址进行通信。
    SOCK_STREAM (Socket Type)是套接字类型（Socket Type）参数，用于指定套接字通信的协议类型。
    SOCK_STREAM 表示创建一个面向连接的套接字，通常与 TCP协议一起使用。
    0是协议（Protocol）参数，用于指定套接字所使用的特定协议。
    当传递 0 作为此参数时，表示让系统自动选择默认的协议。对于 SOCK_STREAM 和 AF_INET 的组合，默认协议通常是 TCP。
    综上所述，socket(AF_INET, SOCK_STREAM, 0) 函数调用将创建一个使用 IPv4 地址族（AF_INET）、
    面向连接的套接字类型（SOCK_STREAM），并自动选择 TCP 协议作为传输协议。
    */
    if (-1 == clientfd)
    {
        std::cerr << "socket create error" << std::endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    /*
    初始化一个sockaddr_in结构体变量server,以便后续使用connect函数来建立到服务器的TCP连接。
    memset函数在这里被用来将server结构体变量中的所有字节设置为0,大小为sockaddr_in。
    */

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    /*
    sockaddr_in结构体用于表示IPv4的套接字地址。这个结构体中包含了一些字段，其中：
    sin_family字段设置为AF_INET，表示这是一个IPv4地址。
    sin_port字段设置为htons(port)，其中htons函数将主机字节序的端口号转换为网络字节序。
    sin_addr字段设置为inet_addr(ip)，其中inet_addr函数将点分十进制的IP地址字符串（如"192.168.1.1"）转换为32位的网络字节序整数。
    */

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        std::cerr << "connect server error" << std::endl;
        close(clientfd);
        exit(-1);
    }
    /*
    在代码中，connect函数被调用，并传入了三个参数：
    clientfd：这是之前创建的客户端套接字的文件描述符。
    (sockaddr *)&server：这是一个指向sockaddr_in结构体变量server的指针，它被强制转换为sockaddr类型的指针，
    因为connect函数接受一个更通用的套接字地址结构指针。
    sizeof(sockaddr_in)：这是sockaddr_in结构体的大小，它告诉connect函数server变量的大小。
    总的来说，这段代码的目的是尝试建立一个TCP连接到指定的服务器IP地址和端口号，并在连接失败时输出错误信息并退出程序。
    */

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        std::cout << "=============================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "=============================" << std::endl;
        std::cout << "choice:";
        int choice = 0;
        std::cin >> choice;
        std::cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[50] = {0};
            std::cout << "userid:";
            std::cin >> id;
            std::cin.get(); // 读掉缓冲区残留的回车
            std::cout << "password:";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            std::string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()), 0);
            if (len == -1)
            {
                std::cerr << "send login msg error:" << request << std::endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    std::cerr << "recv login response error" << std::endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 登录失败
                    {
                        std::cerr << responsejs["errmsg"] << std::endl;
                    }
                    else // 登录成功
                    {
                        // 记录当前用户的id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录当前用户的好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            // 初始化
                            g_currentUserFriendList.clear();

                            std::vector<std::string> vec = responsejs["friends"];
                            for (std::string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 记录当前用户的群组列表信息
                        if (responsejs.contains("groups"))
                        {
                            // 初始化
                            g_currentUserGroupList.clear();

                            std::vector<std::string> vec1 = responsejs["groups"];
                            for (std::string &groupstr : vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                std::vector<std::string> vec2 = grpjs["users"];
                                for (std::string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }

                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        // 显示用户的基本登录信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息  个人聊天信息或者群组消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            std::vector<std::string> vec = responsejs["offlinemsg"];
                            for (std::string &str : vec)
                            {
                                json js = json::parse(str);
                                // time + [id] + name + "said: " + xxx
                                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                                              << " said: " << js["msg"].get<std::string>() << std::endl;
                                }
                                else
                                {
                                    std::cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                                              << " said: " << js["msg"].get<std::string>() << std::endl;
                                }
                            }
                        }

                        // 登录成功，启动接收线程负责接收数据，该线程只启动一次
                        static int readThreadnumber = 0;
                        if (readThreadnumber == 0)
                        {
                            std::thread readTask(readTaskHandler, clientfd); // 在Linux，底层相当于调用pthrad_create
                            readTask.detach();                               // 在Linux，底层相当于调用pthrad_detach
                            readThreadnumber++;
                        }

                        // 进入聊天主菜单页面
                        isMainMenuRunning = true;
                        mainManu(clientfd);
                    }
                }
            }
        }
        break;
        case 2: // register业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            std::cout << "username:";
            std::cin.getline(name, 50);
            std::cout << "userpassword:";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            std::string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            /*
            在这段代码中，send函数被用来发送数据从一个套接字到另一个套接字。
            具体来说，这段代码从一个客户端套接字clientfd发送一个字符串request到服务器。下面是每个参数的解释：
            clientfd: 这是客户端套接字的文件描述符，它表示一个打开的套接字连接。
            request.c_str(): c_str()成员函数返回一个指向正规C字符串的指针。
            strlen(request.c_str()) + 1: 这个表达式计算要发送的字节数。加1是为了包括终止的空字符。
            在网络通信中，通常包括终止的空字符是很重要的，因为它标记了字符串的结束。
            0: 这是send函数的最后一个参数，通常被称为标志。在这里，它被设置为0，表示使用默认的发送选项。
            send函数返回实际发送的字节数。如果返回的值小于你尝试发送的字节数，这通常意味着一个错误发生了，或者连接被对方关闭了。
            */
            if (len == -1)
            {
                std::cerr << "send reg msg error" << request << std::endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    std::cerr << "recv reg response error" << std::endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 注册失败
                    {
                        std::cerr << name << " is already exist, register error!" << std::endl;
                    }
                    else // 注册成功
                    {
                        std::cout << name << " register success, userid is " << responsejs["id"]
                                  << ", do not forget it!" << std::endl;
                    }
                }
            }
        }
        break;
        case 3: // quit业务
            close(clientfd);
            exit(0);
        default:
            std::cerr << "invalid input!" << std::endl;
            break;
        }
    }

    return 0;
}

// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);  // 阻塞了
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接受ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                      << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }
        else if (GROUP_CHAT_MSG == msgtype)
        {
            std::cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                      << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    std::cout << "======================login user========================" << std::endl;
    std::cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << std::endl;
    std::cout << "----------------------friend list-----------------------" << std::endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "----------------------group list------------------------" << std::endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for (GroupUser &user : group.getUsers())
            {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState()
                          << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "==============================================" << std::endl;
}

// "help" command handler
void help(int fd = 0, std::string str = "");
// "chat" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "loginout" command handler
void loginout(int, std::string);

// 系统支持的客户端命令列表
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainManu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer); // 把char*强制转换成string
        std::string command;            // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
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
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainManu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void help(int fd, std::string str)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap)
    {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}
// "addfriend" command handler
void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
}
// "chat" command handler
void chat(int clientfd, std::string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}
// "creategroup" command handler   groupname:groupdesc
void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}
// "groupchat" command handler
void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":"); // groupid:message
    if (-1 == idx)
    {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
}
// "loginout" command handler
void loginout(int clientfd, std::string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send loginout msg error -> " << buffer << std::endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

// 获取系统时间
std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    /*
    这两行代码使用C++标准库中的<chrono>头文件来获取当前的系统时间。
    std::chrono::system_clock::now()获取当前的系统时间点。
    std::chrono::system_clock::to_time_t()将时间点转换为time_t类型，这通常是一个表示自1970年1月1日以来的秒数的整数。
    localtime(&tt)将time_t类型的时间转换为本地时间，并返回一个指向tm结构体的指针。
    */
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    /*
    使用sprintf函数将tm结构体中的时间数据格式化为字符串，并存储在date数组中。
    %d-%02d-%02d %02d:%02d:%02d是格式化字符串，指定了输出时间的格式。
    %d：用于格式化整数。
    %02d：也是用于格式化整数，但有一个额外的 02 指定了宽度和填充字符。
    这意味着数字会被格式化为至少两位，如果只有一位，那么它会在前面用 0 来填充。
    ptm->tm_year + 1900：年份是从1900年开始计数的，所以要加上1900。
    ptm->tm_mon + 1：月份是从0开始计数的，所以要加1。
    ptm->tm_mday、ptm->tm_hour、ptm->tm_min、ptm->tm_sec分别表示天、小时、分钟和秒。
    */
    return std::string(date);
}