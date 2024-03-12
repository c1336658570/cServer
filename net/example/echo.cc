// g++ echo.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <iostream>
#include <functional>
#include <string>
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"

using namespace std::placeholders;

class CharServer
{
public:
    CharServer(cServer::EventLoop *loop, const cServer::InetAddress &addr) : server_(loop, addr), loop_(loop)
    {
        //连接建立/连接断开都会触发此回调
        server_.setConnectionCallback(std::bind(&CharServer::onConnection, this, _1));
        //读事件发生触发此回调
        server_.setMessageCallback(std::bind(&CharServer::onMessage, this, _1, _2, _3));
    }

	void setThreadNum(int num)
	{
	    //设置底层创建的EventLoop个数,不包括base_loop
        server_.setThreadNum(num);
	}

    void start()
    {
        server_.start();
    }

    void onConnection(const cServer::TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            std::cout << conn->peerAddress().toHostPort() << std::endl;
            std::cout << "new conn ... " << std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toHostPort() << std::endl;
            std::cout << "conn close ... " << std::endl;
        }
    }

    void onMessage(const cServer::TcpConnectionPtr &conn,
                   cServer::Buffer *buf,
                   cServer::Timestamp ts)
    {
        std::string recv = buf->retrieveAsString();
        conn->send(recv);
        // conn->shutdown();
    }

private:
    cServer::TcpServer server_;
    cServer::EventLoop *loop_;
};

int main()
{
    cServer::EventLoop loop;

    CharServer server(&loop, {"127.0.0.1", 8080});
    
    server.setThreadNum(3);	//设置SubLoop个数
    server.start(); // epoll_ctl 添加listen_fd
    loop.loop();    //  epoll wait 阻塞等待
}