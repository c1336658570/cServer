// g++ test1.cc ../src/* ../../tool/src/* ../../log/src/*  -I ../include -I ../../tool/include -I ../../log/include/
#include <stdio.h>
#include "EventLoop.h"
#include "Thread.h"

// 在主线程和子线程分别创建一个EventLoop，程序正常运行退出。
void threadFunc() {
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), cServer::CurrentThread::tid());

  cServer::EventLoop loop;
  loop.loop();
}

int main() {
  printf("main(): pid = %d, tid = %d\n", getpid(), cServer::CurrentThread::tid());

  cServer::EventLoop loop;

  cServer::Thread thread(threadFunc);
  thread.start();

  loop.loop();
  pthread_exit(NULL);
}
