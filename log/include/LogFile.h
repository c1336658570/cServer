// 此文件实现将日志内容写入文件中
#ifndef CSERVER_LOG_INCLUDE_LOGFILE_
#define CSERVER_LOG_INCLUDE_LOGFILE_

#include <string>
#include <memory>
#include <unistd.h>
#include "FileUtil.h"
#include "Mutex.h"

namespace cServer {

class LogFile : noncopyable
{
 public:
  // 构造函数，参数包括日志文件的基本名称、滚动大小、是否线程安全、刷新间隔和检查频率
  LogFile(const std::string &basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  // 析构函数，释放资源
  ~LogFile();

  // 追加日志消息到日志文件
  void append(const char *logline, int len);
  // 刷新日志文件
  void flush();
  // 滚动日志文件，基于设定的大小或日期更换日志文件
  bool rollFile();

 private:
  // 无锁版本的追加日志消息到日志文件，用于非线程安全模式的日志记录
  void append_unlocked(const char *logline, int len);

  // 获取日志文件的名称，参数包括基本名称和当前时间，用于生成带有日期、主机、线程ID等信息的日志文件名
  static std::string getLogFileName(const std::string &basename, time_t *now);

  const std::string basename_;    // 日志文件的基本名称，默认保存在当前工作目录下
  const off_t rollSize_;          // 日志文件的滚动大小，日志文件超过设定值进行roll
  const int flushInterval_;       // 刷新间隔，单位秒，用于定时将缓冲区数据写入文件
  const int checkEveryN_;         // 检查频率，每隔N次append操作检查一次是否需要滚动或刷新

  int count_;                     // 对当前日志文件进行的日志操作次数，与checkEveryN_配合判断是否执行滚动或刷新

  std::unique_ptr<MutexLock> mutex_;  // 互斥锁，用于线程安全模式下保护对文件的并发访问
  time_t startOfPeriod_;              // 用于标记同一天的时间戳
  time_t lastRoll_;                   // 上一次roll的时间戳
  time_t lastFlush_;                  // 上一次flush的时间戳
  std::unique_ptr<AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;   // 一天的秒数
};

} // namespace cServer

#endif  // CSERVER_LOG_INCLUDE_LOGFILE_