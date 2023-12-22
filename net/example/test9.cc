// g++ test9.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <stdio.h>

void onConnection(const cServer::TcpConnectionPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(), conn->peerAddress().toHostPort().c_str());
  } else {
    printf("onConnection(): connection [%s] is down\n", conn->name().c_str());
  }
}

void onMessage(const cServer::TcpConnectionPtr& conn, cServer::Buffer* buf,
               cServer::Timestamp receiveTime) {
  printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
         buf->readableBytes(), conn->name().c_str(),
         receiveTime.toFormatString().c_str());

  conn->send(buf->retrieveAsString());
}

int main() {
  printf("main(): pid = %d\n", getpid());

  cServer::InetAddress listenAddr(9981);
  cServer::EventLoop loop;

  cServer::TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.start();

  loop.loop();
}
