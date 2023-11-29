#include <stdio.h>
#include "EventLoop.h"
#include "EventLoopThread.h"

void runInThread() {
  printf("runInThread(): pid = %d, tid = %d\n", getpid(),
         cServer::CurrentThread::tid());
}

int main() {
  printf("main(): pid = %d, tid = %d\n", getpid(), cServer::CurrentThread::tid());

  cServer::EventLoopThread loopThread;
  cServer::EventLoop *loop = loopThread.startLoop();
  loop->runInLoop(runInThread);
  sleep(1);
  loop->runAfter(2, runInThread);
  sleep(3);
  loop->quit();

  printf("exit main().\n");
}
