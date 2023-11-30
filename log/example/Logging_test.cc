// g++ Logging_test.cc ../src/* ../../tool/src/* -I ../include -I ../../tool/include
#include "Logging.h"
#include "LogStream.h"

using namespace cServer;

int main(void) {
  LOG_TRACE << "trace";
  LOG_DEBUG << "debug";
  LOG_INFO << "Hello";
  LOG_WARN << "World";
  LOG_ERROR << "Error";
  LOG_INFO << sizeof(cServer::Logger);
  LOG_INFO << sizeof(cServer::LogStream);
  LOG_INFO << sizeof(cServer::Fmt);
  LOG_INFO << sizeof(cServer::LogStream::Buffer);
}
