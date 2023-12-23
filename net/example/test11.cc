// g++ test11.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <stdio.h>

std::string message;

void onConnection(const cServer::TcpConnectionPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(), conn->peerAddress().toHostPort().c_str());
    conn->send(message);
  } else {
    printf("onConnection(): connection [%s] is down\n", conn->name().c_str());
  }
}

void onWriteComplete(const cServer::TcpConnectionPtr& conn) {
  conn->send(message);
}

void onMessage(const cServer::TcpConnectionPtr& conn, cServer::Buffer* buf,
               cServer::Timestamp receiveTime) {
  printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
         buf->readableBytes(), conn->name().c_str(),
         receiveTime.toFormatString().c_str());

  buf->retrieveAll();
}

int main() {
  printf("main(): pid = %d\n", getpid());

  std::string line;
  for (int i = 33; i < 127; ++i) {
    line.push_back(char(i));
  }
  line += line;

  for (size_t i = 0; i < 127 - 33; ++i) {
    message += line.substr(i, 72) + '\n';
  }

  cServer::InetAddress listenAddr(9981);
  cServer::EventLoop loop;

  cServer::TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.setWriteCompleteCallback(onWriteComplete);
  server.start();

  loop.loop();
}
