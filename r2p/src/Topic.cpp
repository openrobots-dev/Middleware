
#include <r2p/Topic.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/BaseMessage.hpp>

namespace r2p {


bool Topic::has_local_publishers() const {

  SysLock::acquire();
  bool condition = num_local_publishers > 0;
  SysLock::release();
  return condition;
}


bool Topic::has_remote_publishers() const {

  SysLock::acquire();
  bool condition = num_remote_publishers > 0;
  SysLock::release();
  return condition;
}


bool Topic::notify_local(BaseMessage &msg, const Time &timestamp) {

  msg.acquire();

  for (StaticList<LocalSubscriber>::Iterator i = local_subscribers.begin();
       i != local_subscribers.end(); ++i) {
    LocalSubscriber &sub = *i->datap;
    msg.acquire();
    sub.notify(msg, timestamp);
  }

  if (!msg.release()) {
    free(msg);
  }
  return true;
}


bool Topic::notify_remote(BaseMessage &msg, const Time &timestamp) {

  msg.acquire();

  for (StaticList<RemoteSubscriber>::Iterator i = remote_subscribers.begin();
       i != remote_subscribers.end(); ++i) {
    RemoteSubscriber &sub = *i->datap;
    msg.acquire();
    sub.notify(msg, timestamp);
  }

  if (!msg.release()) {
    free(msg);
  }
  return true;
}


void Topic::advertise(LocalPublisher &pub, const Time &publish_timeout) {

  (void)pub;
  SysLock::acquire();
  if (this->publish_timeout > publish_timeout) {
    this->publish_timeout = publish_timeout;
  }
  ++num_local_publishers;
  SysLock::release();
}


void Topic::advertise(RemotePublisher &pub, const Time &publish_timeout) {

  (void)pub;
  SysLock::acquire();
  if (this->publish_timeout > publish_timeout) {
    this->publish_timeout = publish_timeout;
  }
  ++num_remote_publishers;
  SysLock::release();
}


Topic::Topic(const char *namep, size_t type_size)
:
  Named(namep),
  publish_timeout(Time::INFINITE),
  msg_pool(type_size),
  num_local_publishers(0),
  num_remote_publishers(0),
  by_middleware(*this)
{}


}; // namespace r2p
