
#ifndef __R2P__MIDDLEWARE_HPP__
#define __R2P__MIDDLEWARE_HPP__

#include <r2p/common.hpp>
#include <r2p/BaseMessage.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Node.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>
#include <r2p/Thread.hpp>
#include <r2p/Semaphore.hpp>

namespace r2p {

class BaseTransport;
class Time;


class Middleware : public Named, private Uncopyable {
private:
  StaticList<Node> nodes;
  StaticList<Topic> topics;
  StaticList<BaseTransport> transports;

  Topic info_topic;
  Thread *info_threadp;

  Topic boot_topic;

  bool stopped;

public:
  static Middleware instance;

public:
  const StaticList<Node> &get_nodes() const;
  const StaticList<Topic> &get_topics() const;
  const StaticList<BaseTransport> &get_transports() const;
  bool is_stopped() const;

  void initialize(void *info_stackp, size_t info_stacklen,
                  Thread::Priority info_priority);
  void spin();
  void stop();
  void reboot();

  void add(Node &node);
  void add(BaseTransport &transport);
  void add(Topic &topic);

  bool advertise(BasePublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool subscribe_local(BaseSubscriber &sub, const char *namep,
                       BaseMessage msgpool_buf[], size_t msgpool_buflen,
                       size_t type_size);
  bool subscribe_remote(BaseSubscriber &sub, const char *namep,
                        BaseMessage msgpool_buf[], size_t msgpool_buflen,
                        size_t type_size);

  Topic *find_topic(const char *namep);

private:
  Topic *touch_topic(const char *namep, size_t type_size);

private:
  static Thread::ReturnType info_threadf(Thread::ArgumentType);

private:
  Middleware(const char *module_namep, const char *bootloader_namep);
};


inline
const StaticList<Node> &Middleware::get_nodes() const {

  return nodes;
}


inline
const StaticList<Topic> &Middleware::get_topics() const {

  return topics;
}


inline
const StaticList<BaseTransport> &Middleware::get_transports() const {

  return transports;
}


inline
bool Middleware::is_stopped() const {

  SysLock::acquire();
  bool flag = stopped;
  SysLock::release();
  return flag;
}


} // namespace r2p
#endif // __R2P__MIDDLEWARE_HPP__
