#include <cassert>
#include <cstdio>
#include "Logging.h"
#include "FileUtil.h"

namespace cServer {

// 构造函数，用于创建一个 AppendFile 对象，并打开指定的文件
AppendFile::AppendFile(std::string filename) :
  fp_(::fopen(filename.c_str(), "ae")), writtenBytes_(0) {  // 'e'代表O_CLOEXEC
  assert(fp_);                                 // 确保文件成功打开
  ::setbuffer(fp_, buffer_, sizeof(buffer_));  // 设置文件流缓冲区
}

// 析构函数，用于关闭文件
AppendFile::~AppendFile() {
  ::fclose(fp_);  // 关闭文件
}

// 将 logline 指向的内容追加到文件中，len 表示内容的长度
void AppendFile::append(const char* logline, const size_t len) {
  size_t written = 0;

  // 循环直到所有内容都被写入
  while (written != len) {
    size_t remain = len - written;
    size_t n = write(logline + written, remain);  // 调用私有写操作函数
    if (n != remain) {
      int err = ferror(fp_);  // 获取文件错误码
      if (err) {
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));  // 打印错误信息
        break;
      }
    }
    written += n;
  }

  writtenBytes_ += written;   // 更新已写入的字节数
}

// 刷新文件流
void AppendFile::flush() {
  ::fflush(fp_);
}

// 实际执行写操作的私有函数，返回写入的字节数
size_t AppendFile::write(const char* logline, size_t len) {
  return ::fwrite_unlocked(logline, 1, len, fp_);   // 使用无锁版本的 fwrite 函数进行写入
}

}  // namespace cServer
