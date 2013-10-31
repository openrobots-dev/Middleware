
#include <r2p/Topic.hpp>
#include <r2p/Middleware.hpp>
#include <r2p/NamingTraits.hpp>
#include <r2p/Message.hpp>
#include <r2p/Transport.hpp>

namespace r2p {


bool Topic::notify_locals_unsafe(Message &msg, const Time &timestamp) {

  if (has_local_subscribers()) {
    for (StaticList<LocalSubscriber>::IteratorUnsafe i =
         local_subscribers.begin_unsafe();
         i != local_subscribers.end_unsafe(); ++i) {
      msg.acquire_unsafe();
      if (!i->notify_unsafe(msg, timestamp)) {
        msg.release_unsafe();
      }
    }
  }

  return true;
}


bool Topic::notify_remotes_unsafe(Message &msg, const Time &timestamp) {

  if (has_remote_subscribers()) {
    for (StaticList<RemoteSubscriber>::IteratorUnsafe i =
         remote_subscribers.begin_unsafe();
         i != remote_subscribers.end_unsafe(); ++i) {

#if R2P_USE_BRIDGE_MODE
      R2P_ASSERT(i->get_transport() != NULL);
      if (msg.get_source() == i->get_transport()) {
        continue; // Do not send back to source transport
      }
#endif

      msg.acquire_unsafe();
      if (!i->notify_unsafe(msg, timestamp)) {
        msg.release_unsafe();
      }
    }
  }

  return true;
}


bool Topic::forward_unsafe(const Message &msg, const Time &timestamp) {

  bool all = true;

  for (StaticList<RemoteSubscriber>::IteratorUnsafe i =
         remote_subscribers.begin_unsafe();
       i != remote_subscribers.end_unsafe(); ++i) {

#if R2P_USE_BRIDGE_MODE
    R2P_ASSERT(i->get_transport() != NULL);
    if (msg.get_source() == i->get_transport()) {
      continue; // Do not send back to source transport
    }
#endif

    Message *msgp;
    if (alloc_unsafe(msgp)) {
      Message::copy(*msgp, msg, get_type_size());
      patch_pubsub_msg(*msgp, *i->get_transport());
      msgp->acquire_unsafe();
      if (!i->notify_unsafe(*msgp, timestamp)) {
        free_unsafe(*msgp);
        all = false;
      }
    } else {
      all = false;
    }
  }

  return all;
}


Message *Topic::alloc() {

  SysLock::acquire();
  Message *msgp = alloc_unsafe();
  SysLock::release();
  return msgp;
}


bool Topic::notify_locals(Message &msg, const Time &timestamp) {

  if (has_local_subscribers()) {
    for (StaticList<LocalSubscriber>::Iterator i = local_subscribers.begin();
         i != local_subscribers.end(); ++i) {
      msg.acquire();
      if (!i->notify(msg, timestamp)) {
        msg.release();
      }
    }
  }

  return true;
}


bool Topic::notify_remotes(Message &msg, const Time &timestamp) {

  { SysLock::Scope lock;
  if (!has_remote_subscribers()) return true; }

  for (StaticList<RemoteSubscriber>::Iterator i = remote_subscribers.begin();
       i != remote_subscribers.end(); ++i) {

#if R2P_USE_BRIDGE_MODE
    { SysLock::Scope lock;
    R2P_ASSERT(i->get_transport() != NULL);
    if (msg.get_source() == i->get_transport()) {
      continue; // Do not send back to source transport
    } }
#endif

    msg.acquire();
    if (!i->notify(msg, timestamp)) {
      msg.release();
    }
  }

  return true;
}


bool Topic::forward(const Message &msg, const Time &timestamp) {

  bool all = true;

  for (StaticList<RemoteSubscriber>::Iterator i = remote_subscribers.begin();
       i != remote_subscribers.end(); ++i) {

#if R2P_USE_BRIDGE_MODE
    R2P_ASSERT(i->get_transport() != NULL);
    if (msg.get_source() == i->get_transport()) {
      continue; // Do not send back to source transport
    }
#endif

    Message *msgp;
    if (alloc(msgp)) {
      Message::copy(*msgp, msg, get_type_size());
      patch_pubsub_msg(*msgp, *i->get_transport());
      msgp->acquire();
      if (!i->notify(*msgp, timestamp)) {
        free(*msgp);
        all = false;
      }
    } else {
      all = false;
    }
  }

  return all;
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


void Topic::patch_pubsub_msg(Message &msg, Transport &transport) const {

  if (this != &Middleware::instance.get_mgmt_topic()) return;

  MgmtMsg &mgmt_msg = static_cast<MgmtMsg &>(msg);
  switch (mgmt_msg.type) {
  case MgmtMsg::ADVERTISE:
  case MgmtMsg::SUBSCRIBE_REQUEST:
  case MgmtMsg::SUBSCRIBE_RESPONSE: {
    transport.fill_raw_params(*this, mgmt_msg.pubsub.raw_params);
    break;
  }
  default: break;
  }
}


Topic::Topic(const char *namep, size_t type_size, bool forwarding)
:
  namep(namep),
  publish_timeout(Time::INFINITE),
  msg_pool(type_size),
  num_local_publishers(0),
  num_remote_publishers(0),
  max_queue_length(0),
#if R2P_USE_BRIDGE_MODE
  forwarding(forwarding),
#endif
  by_middleware(*this)
{
  R2P_ASSERT(is_identifier(namep, NamingTraits<Topic>::MAX_LENGTH));

  (void)forwarding;
}


}; // namespace r2p
