
#include <r2p/Topic.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>

#include <cstring>

namespace r2p {


size_t Topic::get_num_publishers() const {

  SysLock::acquire();
  size_t n = num_publishers;
  SysLock::release();
  return n;
}


bool Topic::notify_locally(BaseMessage &msg, const Time &timestamp) {

  msg.acquire();

  for (StaticList<BaseSubscriber>::Iterator i = local_subscribers.begin();
       i != local_subscribers.end(); ++i) {
    BaseSubscriber &sub = *i->datap;
    msg.acquire();
    sub.notify(msg, timestamp);
  }

  if (!msg.release()) {
    free(msg);
  }
  return true;
}


bool Topic::notify_remotely(BaseMessage &msg, const Time &timestamp) {

  msg.acquire();

  for (StaticList<BaseSubscriber>::Iterator i = remote_subscribers.begin();
       i != remote_subscribers.end(); ++i) {
    BaseSubscriber &sub = *i->datap;
    msg.acquire();
    sub.notify(msg, timestamp);
  }

  if (!msg.release()) {
    free(msg);
  }
  return true;
}


void Topic::advertise_cb(BasePublisher &pub, const Time &publish_timeout) {

  (void)pub;
  SysLock::acquire();
  if (this->publish_timeout > publish_timeout) {
    this->publish_timeout = publish_timeout;
  }
  ++num_publishers;
  SysLock::release();
}


Topic::Topic(const char *namep, size_t type_size)
:
  Named(namep),
  publish_timeout(Time::INFINITE),
  msg_pool(type_size),
  num_publishers(0),
  by_middleware(*this)
{}


}; // namespace r2p
