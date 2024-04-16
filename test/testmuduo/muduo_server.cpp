
/*
muduo网络库给用户提供了两个主要的类
TcpServer : 用于编写服务器程序的
TcpClient : 用于编写客户端程序的

epoll + 线程池
好处：能够把网络I/O的代码和业务代码区分开
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>
using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;

/*基于muduo网路库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpSserver事件循环对象的指针，输出ChatServer的构造函数
4. 在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程

*/

class ChatServer
{
public:
    ChatServer(EventLoop *loop, // 事件循环
               const InetAddress &listenAddr,
               const std::string &nameArg)
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器的线程数量 1个I/O 3个worker线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门处理用户的连接和断开   epoll listenfd acceptor
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            std::cout << conn->peerAddress().toIpPort() << 
                "->" << conn->localAddress().toIpPort() << "state:online" << std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toIpPort() << 
                "->" << conn->localAddress().toIpPort() << "state:offline" << std::endl;
            conn->shutdown(); // close(fd)
            // _loop->quit();
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,                 // 缓冲区
                   Timestamp time)               // 接受到数据的时间信息
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data: " << buf << "time: " << time.toString() << std::endl;
        conn->send(buf);
    }

    TcpServer _server; // #1
    EventLoop *_loop;  // #2 epoll
};

int main() 
{
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl=>epoll
    loop.loop();   // epoll_wait以阻塞方式等待新用户连接，已连接用户读写事件等

    return 0;
}