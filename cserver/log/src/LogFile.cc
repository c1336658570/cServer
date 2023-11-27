#include <cassert>
#include "LogFile.h"

namespace cServer {

// 构造函数，初始化 LogFile 对象
LogFile::LogFile(const std::string &basename, off_t rollSize, bool threadSafe,
    int flushInterval, int checkEveryN) :
    basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0) {
  assert(basename.find('/') == std::string::npos);
  rollFile();
}

// 析构，默认实现
LogFile::~LogFile() = default;

// 在加锁或者不加锁的情况下追加日志内容到文件
void LogFile::append(const char *logline, int len) {
  if (mutex_) {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  } else {
    append_unlocked(logline, len);
  }
}

// 在加锁或者不加锁的情况下刷新文件流
void LogFile::flush() {
  if (mutex_) {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  } else {
    file_->flush();
  }
}

// 在加锁或者不加锁的情况下追加日志内容到文件，并根据条件执行滚动操作
void LogFile::append_unlocked(const char *logline, int len) {
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_) {  // 如果已写入字节数超过滚动大小
    rollFile();   // 滚动
  } else {
    ++count_;
    if (count_ >= checkEveryN_) {   // 如果写入次数达到检查次数
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_) {  // 如果当前时间不在同一天
        rollFile();   // 滚动
      } else if (now - lastFlush_ > flushInterval_) {   // 如果距离上次刷新的时间超过刷新间隔
        lastFlush_ = now;
        file_->flush();       // 刷新文件流
      }
    }
  }
}

// 执行日志文件滚动操作
bool LogFile::rollFile() {
  time_t now = 0;
  std::string filename = getLogFileName(basename_, &now);     // 获取新的日志文件名
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_) {    // 如果当前时间大于上次滚动时间
    lastRoll_ = now;        // 更新上一次roll时间
    lastFlush_ = now;       // 更新上一次flush时间
    startOfPeriod_ = start; // 标记哪一天的日志
    file_.reset(new AppendFile(filename));    // 创建新的 AppendFile 对象，并释放上一次的 AppendFile 对象，上一次的 AppendFile 对象会执行析构
    return true;
  }
  return false;
}

// 获取日志文件名，文件名包含基本名称、时间戳、主机名和进程ID和.log。
std::string LogFile::getLogFileName(const std::string &basename, time_t *now) {
  std::string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm);  // 获取当前时间的tm结构体
  // %Y: 四位数的年份（例如：2023）。%m: 两位数的月份（01至12）。%d: 两位数的日期（01至31）。
  // %H: 24小时制的小时数（00至23）。%M: 分钟数（00至59）。%S: 秒数（00至59）。
  strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);    // 格式化时间字符串
  filename += timebuf;

  char buf[256];
  if (::gethostname(buf, sizeof(buf)) == 0) {    // 获取主机名
    buf[sizeof(buf) - 1] = '\0';
    filename += buf;
  } else {
    filename += "unknownhost";
  }

  char pidbuf[32];
  snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid());   // 获取进程ID
  filename += pidbuf;

  filename += ".log";

  return filename;
}

}  // namespace cServer