// 此文件实现日志库的前端接口，即网络库可以直接使用LOG_TRACE等宏，用法如下LOG_TRACE << "hello" << std::endl;
#ifndef CSERVER_LOG_INCLUDE_LOGGING_
#define CSERVER_LOG_INCLUDE_LOGGING_

#include <cstring>
#include <string>
#include "LogStream.h"
#include "Timestamp.h"

namespace cServer {

class Logger {
 public:
  // 源文件名（不带路径）(basename)
  class SourceFile {
   public:
    // 构造函数，接受字符数组，提取不带路径的文件名
    template <int N>
    SourceFile(const char (&arr)[N]) : data_(arr), size_(N-1) {
      const char *slash = strrchr(data_, '/');
      if (slash) {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    // 显式构造函数，接受文件名，提取不带路径的文件名
    explicit SourceFile(const char *filename) : data_(filename) {
      const char *slash = strrchr(filename, '/');
      if (slash) {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    // 返回不带路径的文件名
    const char *data() const {
      return data_;
    }

    // 返回文件名长度
    int size() const {
      return size_;
    }
  
   private:
    const char *data_;    // 文件名
    int size_;            // 文件名长度
  };

  // 日志等级
  enum LogLevel {
    TRACE,            // 追踪详细信息
    DEBUG,            // 调试
    INFO,             // 一般性信息
    WARN,             // 警告
    ERROR,            // 错误
    FATAL,            // 致命错误
    NUM_LOG_LEVELS,   // 日志等级的数量
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char *func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream &stream() {
    return impl_.stream_;
  }

  static LogLevel logLevel();               // 获取日志等级
  static void setLogLevel(LogLevel level);  // 设置日志等级

  typedef void (*OutputFunc)(const char *msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);        // 设置输出刷新函数
  static void setFlush(FlushFunc);          // 设置刷新函数
 
 private:
  class Impl {
   public:
    typedef Logger::LogLevel LogLevel;
    // 构造函数，接受日志等级、错误号、源文件、行号
    Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
    // 格式化时间戳为字符串，并将其附加到日志流中
    void formatTime();
    void finish();            // 一条日志的结束

    Timestamp time_;          // 记录日志时间戳
    LogStream stream_;        // 日志缓存流
    LogLevel level_;          // 日志级别
    int line_;                // 当前记录日志宏的源代码行号
    SourceFile basename_;     // 当前记录日志宏的源代码名称
  };

  Impl impl_;
};

extern Logger::LogLevel g_logLevel;   // 全局日志等级

// 获取全局日志等级
inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

// LOG_TRACE 宏，记录 TRACE 级别的日志
#define LOG_TRACE if (cServer::Logger::logLevel() <= cServer::Logger::TRACE) \
  cServer::Logger(__FILE__, __LINE__, cServer::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (cServer::Logger::logLevel() <= cServer::Logger::DEBUG) \
  cServer::Logger(__FILE__, __LINE__, cServer::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (cServer::Logger::logLevel() <= cServer::Logger::INFO) \
  cServer::Logger(__FILE__, __LINE__).stream()
  
#define LOG_WARN cServer::Logger(__FILE__, __LINE__, cServer::Logger::WARN).stream()
#define LOG_ERROR cServer::Logger(__FILE__, __LINE__, cServer::Logger::ERROR).stream()
#define LOG_FATAL cServer::Logger(__FILE__, __LINE__, cServer::Logger::FATAL).stream()

// LOG_SYSERR 宏，记录 ERROR 级别的日志，但不中止程序
#define LOG_SYSERR cServer::Logger(__FILE__, __LINE__, false).stream()
// LOG_SYSFATAL 宏，记录 FATAL 级别的日志，中止程序
#define LOG_SYSFATAL cServer::Logger(__FILE__, __LINE__, true).stream()

const char *strerror_tl(int savedErrno);

} // namespace cServer

#endif  // CSERVER_LOG_INCLUDE_LOGGING_