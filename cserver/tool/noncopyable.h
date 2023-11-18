#ifndef CSERVER_TOOL_NONCOPYABLE_
#define CSERVER_TOOL_NONCOPYABLE_

namespace cserver {

// 禁止拷贝构造和拷贝赋值，禁止用户创建该类，允许派生类创建。
class noncopyable {
 public:
  noncopyable(const noncopyable &) = delete;
  void operator=(const noncopyable &) = delete;
 
 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

} // namespace cserver

#endif  // CSERVER_TOOL_NONCOPYABLE_