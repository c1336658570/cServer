#include "Logging.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include <cassert>

namespace cServer {
  __thread char t_errnoBuf[512];    // 声明了一个线程局部存储的字符数组
  __thread char t_time[64];         // 存储时间信息的字符串
  __thread time_t t_lastSecond;     // 当前线程上一次日志记录时的秒数

// 返回给定错误码（savedErrno）对应的错误信息字符串。
const char* strerror_tl(int savedErrno) {
  return strerror_r(savedErrno, t_errnoBuf, sizeof(t_errnoBuf));
}

// 初始化日志级别，根据环境变量的设置来确定日志的初始级别。
Logger::LogLevel initLogLevel()
{
  // 使用 ::getenv 函数获取环境变量的值，用于动态配置日志级别。
  // 如果环境变量 "CSERVER_LOG_TRACE" 存在，则设置日志级别为 TRACE。
  if (::getenv("CSERVER_LOG_TRACE")) {
    return Logger::TRACE;
  }
  // 如果环境变量 "CSERVER_LOG_DEBUG" 存在，则设置日志级别为 DEBUG。
  else if (::getenv("CSERVER_LOG_DEBUG")) {
    return Logger::DEBUG;
  }
  else {
    return Logger::INFO;
  }
}

// 设置初始时的日志级别
Logger::LogLevel g_logLevel = initLogLevel();

// 定义日志级别对应的字符串数组，用于在日志输出中标识日志级别。
const char *LogLevelName[Logger::NUM_LOG_LEVELS] = {
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

// 格式化时间戳为字符串，并将其附加到日志流中
void Logger::Impl::formatTime() {
  // 获取时间戳的微秒数
  int64_t microsecondsSinceEpoch = time_.microsecondsSinceEpoch();
  // 将微秒数转换为秒数和微秒数
  auto seconds = static_cast<time_t>(microsecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microsecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
  // 如果秒数发生变化，则更新 t_lastSecond 和 t_time
  if (seconds != t_lastSecond) {
    t_lastSecond = seconds;
    struct tm t;
    // 将秒数转换为本地时间结构
    ::localtime_r(&seconds, &t);

    // 格式化输出年月日时分秒
    int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                        t.tm_year + 1900,
                        t.tm_mon + 1,
                        t.tm_mday,
                        t.tm_hour,
                        t.tm_min,
                        t.tm_sec);
    assert(len == 17);    // 确保字符串长度为17，包括空字符'\0'
  }
  // 格式化微秒数，并附加到日志流中
  Fmt us(".%06dZ ", microseconds);
  assert(us.length() == 9);     // 确保字符串长度为9，包括空字符'\0'
  stream_.append(t_time, 17);   // 附加年月日时分秒
  stream_.append(us.data(), 9); // 附加微秒数
}

Logger::Impl::Impl(LogLevel level, int old_errno, const SourceFile &file, int line)
  : time_(Timestamp::now()),    // 获取当前时间戳
    stream_(),                  // 初始化日志流
    level_(level),              // 设置日志级别
    line_(line),                // 设置源代码行号
    basename_(file) {           // 设置源文件名
  formatTime();                 // 格式化时间戳
  CurrentThread::tid();         // 获取当前线程ID
  stream_.append(CurrentThread::tidString(), CurrentThread::t_tidLen);  // 写入线程ID
  stream_.append(LogLevelName[level], 6);   // 写入日志级别
  if (old_errno != 0) {
      stream_ << strerror_tl(old_errno) << " (errno=" << old_errno << ") ";   // 写入错误信息
  }
}

// 默认的日志输出函数，将日志消息写入标准输出
void defaultOutput(const char *msg, int len) {
  size_t n = fwrite(msg, 1, len, stdout);
  (void)n;
}

// 默认的日志刷新函数，刷新标准输出缓冲区
void defaultFlush() {
  fflush(stdout);
}

// 全局变量，用于存储日志输出和刷新函数的指针，默认为标准输出和标准刷新
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

// 实现类的成员函数，用于在日志消息结尾添加文件名和行号
void Logger::Impl::finish(){
  stream_ << " - " << basename_.data() << ':' << line_ << '\n';
}

// 构造函数，初始化日志实现类，设置日志级别为INFO
Logger::Logger(SourceFile file, int line) 
  : impl_(INFO, 0, file, line) {
}

// 构造函数，初始化日志实现类，设置日志级别为指定级别，附带函数名信息
Logger::Logger(SourceFile file, int line, LogLevel level, const char *func) 
  : impl_(level, 0, file, line) {
  impl_.stream_ << func << ' ';
}

// 构造函数，初始化日志实现类，设置日志级别为指定级别
Logger::Logger(SourceFile file, int line, LogLevel level)
  : impl_(level, 0, file, line) {
}

// 构造函数，初始化日志实现类，设置日志级别为FATAL或ERROR，附带错误信息
Logger::Logger(SourceFile file, int line, bool toAbort)
  : impl_(toAbort ? FATAL : ERROR, errno, file, line) {
}

// 析构函数，完成日志消息并输出到指定设备，如果级别为FATAL，则刷新缓冲区并终止程序
Logger::~Logger() {
  impl_.finish();   // 完成日志消息
  const LogStream::Buffer &buf(stream().buffer());
  g_output(buf.data(), buf.length());   // 输出日志消息
  if (impl_.level_ == FATAL)
  {
    g_flush();    // 刷新输出缓冲区
    abort();      // 终止程序
  }
}

// 设置全局日志级别
void Logger::setLogLevel(Logger::LogLevel level) {
  g_logLevel = level;
}

// 设置全局日志输出函数
void Logger::setOutput(OutputFunc out) {
  g_output = out;
}

// 设置全局日志刷新函数
void Logger::setFlush(FlushFunc flush) {
  g_flush = flush;
}

} // namespace cServer
