#ifndef CSERVER_TOOL_INCLUDE_FileUtil_
#define CSERVER_TOOL_INCLUDE_FileUtil_

#include <string>
#include "noncopyable.h"

namespace cServer {

class AppendFile : noncopyable {
 public:
  // 创建一个AppendFile对象，并打开指定的文件（a:追加，e，相当于O_CLOEXEC，，在调用exec是会关闭文件描述符）
  explicit AppendFile(std::string filename);

  // 析构函数，用于关闭文件
  ~AppendFile();

  // 将logline指向的内容追加到缓冲区（flush后会进入文件）
  void append(const char *logline, size_t len);

  // 刷新文件流
  void flush();

  // 返回已写入的字节数
  off_t writtenBytes() const {
    return writtenBytes_;
  }

 private:
 // 实际执行写操作的函数，返回写入的字节数
  size_t write(const char* logline, size_t len);

  FILE *fp_;                  // 文件指针，用于操作文件流
  char buffer_[64 * 1024];    // 文件输出缓冲区，用于缓存写入的数据
  off_t writtenBytes_;        // 已写入的字节数
};

}  // namespace cServer

#endif  // CSERVER_TOOL_INCLUDE_FileUtil_