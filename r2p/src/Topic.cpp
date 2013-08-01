
#include <r2p/Topic.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Message.hpp>

namespace r2p {


bool Topic::notify_locals_unsafe(Message &msg, const Time &timestamp) {

  if (has_local_subscribers()) {
    msg.acquire_unsafe();

    for (StaticList<LocalSubscriber>::IteratorUnsafe i =
         local_subscribers.begin_unsafe();
         i != local_subscribers.end_unsafe(); ++i) {
      msg.acquire_unsafe();
      i->itemp->notify_unsafe(msg, timestamp);
    }

    if (!msg.release_unsafe()) {
      free_unsafe(msg);
    }
  } else {
    free_unsafe(msg);
  }
  return true;
}


bool Topic::notify_remotes_unsafe(Message &msg, const Time &timestamp) {

  if (has_remote_subscribers()) {
    msg.acquire_unsafe();

    for (StaticList<RemoteSubscriber>::IteratorUnsafe i =
         remote_subscribers.begin_unsafe();
         i != remote_subscribers.end_unsafe(); ++i) {
      msg.acquire_unsafe();
      i->itemp->notify_unsafe(msg, timestamp);
    }

    if (!msg.release_unsafe()) {
      free_unsafe(msg);
    }
  } else {
    free_unsafe(msg);
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


void Topic::subscribe(LocalSubscriber &sub, size_t queue_length) {

  SysLock::acquire();
  if (max_queue_length < queue_length) {
    max_queue_length = queue_length;
  }
  local_subscribers.link_unsafe(sub.by_topic);
  SysLock::release();
}


void Topic::subscribe(RemoteSubscriber &sub, size_t queue_length) {

  SysLock::acquire();
  if (max_queue_length < queue_length) {
    max_queue_length = queue_length;
  }
  remote_subscribers.link_unsafe(sub.by_topic);
  SysLock::release();
}


Topic::Topic(const char *namep, size_t type_size)
:
  namep(namep),
  publish_timeout(Time::INFINITE),
  msg_pool(type_size),
  num_local_publishers(0),
  num_remote_publishers(0),
  max_queue_length(0),
  by_middleware(*this)
{
  R2P_ASSERT(is_identifier(namep));
  R2P_ASSERT(::strlen(namep) <= NamingTraits<Topic>::MAX_LENGTH);
}


}; // namespace r2p
