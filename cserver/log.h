#ifndef __CSERVER_LOG_H__
#define __CSERVER_LOG_H__

#include <string.h>
#include <cstdint>
#include <string>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>

namespace cserver{

class LogLevel {
public:
  // 日志级别
  enum Level{
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };
};

// 日志事件
class LogEvent {
public:
  typedef std::shared_ptr<LogEvent> ptr;
  LogEvent();
private:
  const char *m_file = nullptr; // 文件名
  int32_t m_line = 0;           // 行号
  uint32_t m_elapse = 0;        // 程序启动到现在的毫秒数
  uint32_t m_threadId = 0;      // 线程id
  uint32_t m_fiberId = 0;       // 协程id
  uint64_t m_time = 0;          // 时间戳
  std::string m_content;        // 消息
};

// 日志格式器
class LogFormatter {
public:
  typedef std::shared_ptr<LogFormatter> ptr;
  LogFormatter(std::string &pattern);

  // %t(时间) %thread_id(线程id)%m(消息)%n(换行)
  std::string format(LogEvent::ptr event);

private:
  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;
    virtual ~FormatItem() {}

    virtual std::string format(LogEvent::ptr event) = 0;
  };

private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
};

// 日值输出目的地
class LogAppender {
public:
  typedef std::shared_ptr<LogAppender> ptr;
  virtual ~LogAppender() {};
  
  virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

  void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
  LogFormatter::ptr getFomatter() const { return m_formatter; }
protected:
  LogLevel::Level m_level;
  LogFormatter::ptr m_formatter;
};

// 日志器
class Logger {
public:
  typedef std::shared_ptr<Logger> ptr;
  Logger(const std::string &name = "root");

  void Log(LogLevel::Level level, LogEvent::ptr event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);  // 添加Appender
  void delAppender(LogAppender::ptr appender);  // 删除Appender
  LogLevel::Level getLevel() const { return m_level; }  // 获取日志级别
  void setLevel(LogLevel::Level val) { m_level = val; } // 设置日志级别
private:
  std::string m_name;       // 日志名称
  LogLevel::Level m_level;  // 日志级别
  std::list<LogAppender::ptr> m_appenders;  // Appender是列表，输出到目的地集合
};

// 输出到控制台
class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;
  void log(LogLevel::Level level, LogEvent::ptr event) override;
private:
};

// 输出到文件
class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;
  FileLogAppender(const std::string &filename);
  void log(LogLevel::Level level, LogEvent::ptr event) override;

  bool reopen();  // 重新打开文件，成功返回true
private:
  std::string m_filename;
  std::ofstream m_filestream;
};

}


#endif
