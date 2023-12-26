// g++ test13.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include <utility>
#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpClient.h"
#include "Logging.h"

std::string message = "Hello\n";

void onConnection(const cServer::TcpConnectionPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(), conn->peerAddress().toHostPort().c_str());
    conn->send(message);
  } else {
    printf("onConnection(): connection [%s] is down\n", conn->name().c_str());
  }
}

void onMessage(const cServer::TcpConnectionPtr& conn, cServer::Buffer* buf,
               cServer::Timestamp receiveTime) {
  printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
         buf->readableBytes(), conn->name().c_str(),
         receiveTime.toFormatString().c_str());

  printf("onMessage(): [%s]\n", buf->retrieveAsString().c_str());
}

int main() {
  cServer::EventLoop loop;
  cServer::InetAddress serverAddr("localhost", 9981);
  cServer::TcpClient client(&loop, serverAddr);

  client.setConnectionCallback(onConnection);
  client.setMessageCallback(onMessage);
  client.enableRetry();
  client.connect();
  loop.loop();
}
