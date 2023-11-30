// g++ test7.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <stdio.h>
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"

void newConnection(int sockfd, const cServer::InetAddress& peerAddr) {
  printf("newConnection(): accepted a new connection from %s\n", peerAddr.toHostPort().c_str());
  ::write(sockfd, "How are you?\n", 13);
  close(sockfd);
}

int main() {
  printf("main(): pid = %d\n", getpid());

  cServer::InetAddress listenAddr(9981);
  cServer::EventLoop loop;

  cServer::Acceptor acceptor(&loop, listenAddr);
  acceptor.setNewConnectionCallback(newConnection);
  acceptor.listen();

  loop.loop();
}
