#pragma once

#include <r2p/common.hpp>
#include <r2p/StaticList.hpp>
#include <r2p/Topic.hpp>
#include <r2p/Thread.hpp>
#include <r2p/InfoMsg.hpp>
#include <r2p/Publisher.hpp>
#include <r2p/Subscriber.hpp>
#include <r2p/Node.hpp>

namespace r2p {

class Node;
class Transport;
class Time;
class Topic;
class LocalPublisher;
class LocalSubscriber;
class RemotePublisher;
class RemoteSubscriber;


class Middleware : private Uncopyable {
private:
  const char *const module_namep;
  StaticList<Node> nodes;
  StaticList<Topic> topics;
  StaticList<Transport> transports;

  enum { INFO_BUFFER_LENGTH = 5 };
  Topic info_topic;
  Thread *info_threadp;
  enum { INFO_TIMEOUT_MS = 20 };

  Topic boot_topic;

  StaticList<Topic>::Iterator topic_iter;
  Time topic_lastiter_time;
  enum { TOPIC_CHECK_TIMEOUT_MS = 100 };

  bool stopped;

public:
  static Middleware instance;

public:
  const char *get_module_name() const;
  const StaticList<Node> &get_nodes() const;
  const StaticList<Topic> &get_topics() const;
  const StaticList<Transport> &get_transports() const;
  bool is_stopped() const;

  void initialize(void *info_stackp, size_t info_stacklen,
                  Thread::Priority info_priority);
  void spin();
  void stop();
  void reboot();

  void add(Node &node);
  void add(Transport &transport);
  void add(Topic &topic);

  bool advertise(LocalPublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool advertise(RemotePublisher &pub, const char *namep,
                 const Time &publish_timeout, size_t type_size);
  bool subscribe(LocalSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msgpool_buflen,
                 size_t type_size);
  bool subscribe(RemoteSubscriber &sub, const char *namep,
                 Message msgpool_buf[], size_t msgpool_buflen,
                 size_t type_size);

  Topic *find_topic(const char *namep);

private:
  Topic *touch_topic(const char *namep, size_t type_size);

private:
  static Thread::Return info_threadf(Thread::Argument);
  void do_info_thread();

private:
  Middleware(const char *module_namep, const char *bootloader_namep);
};


inline
const char *Middleware::get_module_name() const {

  return module_namep;
}


inline
const StaticList<Node> &Middleware::get_nodes() const {

  return nodes;
}


inline
const StaticList<Topic> &Middleware::get_topics() const {

  return topics;
}


inline
const StaticList<Transport> &Middleware::get_transports() const {

  return transports;
}


inline
bool Middleware::is_stopped() const {

  return stopped;
}


} // namespace r2p
