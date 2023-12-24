// g++ test8.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <cstdio>
#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpServer.h"

void onConnection(const cServer::TcpConnectionPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): tid=%d new connection [%s] from %s\n",
           cServer::CurrentThread::tid(), conn->name().c_str(),
           conn->peerAddress().toHostPort().c_str());
  } else {
    printf("onConnection(): tid=%d connection [%s] is down\n",
           cServer::CurrentThread::tid(), conn->name().c_str());
  }
}

void onMessage(const cServer::TcpConnectionPtr& conn, cServer::Buffer* buf,
               cServer::Timestamp receiveTime) {
  printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
         cServer::CurrentThread::tid(), buf->readableBytes(),
         conn->name().c_str(), receiveTime.toFormatString().c_str());

  printf("onMessage(): [%s]\n", buf->retrieveAsString().c_str());
}

int main(int argc, char* argv[]) {
  printf("main(): pid = %d\n", getpid());

  cServer::InetAddress listenAddr(9981);
  cServer::EventLoop loop;

  cServer::TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  if (argc > 1) {
    server.setThreadNum(atoi(argv[1]));
  }
  server.start();

  loop.loop();
}
