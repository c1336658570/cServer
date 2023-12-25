// g++ test12.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <stdio.h>
#include "Connector.h"
#include "EventLoop.h"

cServer::EventLoop *g_loop;

void connectCallback(int sockfd) {
  printf("connected.\n");
  g_loop->quit();
}

int main(int argc, char* argv[]) {
  cServer::EventLoop loop;
  g_loop = &loop;
  cServer::InetAddress addr("127.0.0.1", 9981);
  cServer::ConnectorPtr connector(new cServer::Connector(&loop, addr));
  connector->setNewConnectionCallback(connectCallback);
  connector->start();

  loop.loop();
}
