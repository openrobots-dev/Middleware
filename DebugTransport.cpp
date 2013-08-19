
#include "DebugTransport.hpp"
#include "DebugPublisher.hpp"
#include "DebugSubscriber.hpp"
#include <r2p/Middleware.hpp>

#include <cstring>

namespace r2p {


inline
static bool send_char(BaseChannel *channelp, char c,
                      systime_t timeout = TIME_INFINITE) {

  return chnPutTimeout(channelp, c, timeout) == Q_OK;
}


inline
static bool expect_char(BaseChannel *channelp, char c,
                        systime_t timeout = TIME_INFINITE) {

  return chnGetTimeout(channelp, timeout) == c;
}


static bool recv_char(BaseChannel *channelp, char &c,
                      systime_t timeout = TIME_INFINITE) {

  register msg_t value = chnGetTimeout(channelp, timeout);
  if ((value & ~0xFF) == 0) {
    c = static_cast<char>(value);
    return true;
  }
  return false;
}


inline
static bool send_byte(BaseChannel *channelp, uint8_t byte,
                      systime_t timeout = TIME_INFINITE) {

  static const char hex[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
  };

  if (chnPutTimeout(channelp, hex[byte >> 4], timeout) != Q_OK) return false;
  if (chnPutTimeout(channelp, hex[byte & 15], timeout) != Q_OK) return false;
  return true;
}


static bool recv_byte(BaseChannel *channelp, uint8_t &byte,
                      systime_t timeout = TIME_INFINITE) {

  msg_t c;

  c = chnGetTimeout(channelp, timeout);
  if        ((c >= '0' && c <= '9')) {
    byte = (static_cast<uint8_t>(c) - '0') << 4;
  } else if ((c >= 'A' && c <= 'F')) {
    byte = (static_cast<uint8_t>(c) + (10 - 'A')) << 4;
  } else if ((c >= 'a' && c <= 'f')) {
    byte = (static_cast<uint8_t>(c) + (10 - 'a')) << 4;
  } else {
    return false;
  }

  c = chnGetTimeout(channelp, timeout);
  if        ((c >= '0' && c <= '9')) {
    byte |= static_cast<uint8_t>(c) - '0';
  } else if ((c >= 'A' && c <= 'F')) {
    byte |= static_cast<uint8_t>(c) + (10 - 'A');
  } else if ((c >= 'a' && c <= 'f')) {
    byte |= static_cast<uint8_t>(c) + (10 - 'a');
  } else {
    return false;
  }

  return true;
}


static bool send_chunk(BaseChannel *channelp,
                       const void *chunkp, size_t length,
                       systime_t timeout = TIME_INFINITE) {

  const uint8_t *p = reinterpret_cast<const uint8_t *>(chunkp);
  while (length-- > 0) {
    if (!send_byte(channelp, *p++, timeout)) return false;
  }
  return true;
}


static bool recv_chunk(BaseChannel *channelp,
                       void *chunkp, size_t length,
                       systime_t timeout = TIME_INFINITE) {

  uint8_t *p = reinterpret_cast<uint8_t *>(chunkp);
  while (length-- > 0) {
    if (!recv_byte(channelp, *p++, timeout)) return false;
  }
  return true;
}


static bool send_chunk_rev(BaseChannel *channelp,
                           const void *chunkp, size_t length,
                           systime_t timeout = TIME_INFINITE) {

  for (const uint8_t *p = reinterpret_cast<const uint8_t *>(chunkp) + length - 1;
       length-- > 0; --p) {
    if (!send_byte(channelp, *p, timeout)) return false;
  }
  return true;
}


static bool recv_chunk_rev(BaseChannel *channelp,
                           void *chunkp, size_t length,
                           systime_t timeout = TIME_INFINITE) {

  for (uint8_t *p = reinterpret_cast<uint8_t *>(chunkp) + length - 1;
       length-- > 0; --p) {
    if (!recv_byte(channelp, *p, timeout)) return false;
  }
  return true;
}


static bool send_string(BaseChannel *channelp,
                        const char *stringp, size_t length,
                        systime_t timeout = TIME_INFINITE) {

  for (; length-- > 0; ++stringp) {
    if (!send_char(channelp, *stringp, timeout)) return false;
  }
  return true;
}


static bool recv_string(BaseChannel *channelp,
                        char *stringp, size_t length,
                        systime_t timeout = TIME_INFINITE) {

  for (; length-- > 0; ++stringp) {
    if (!recv_char(channelp, *stringp, timeout)) return false;
  }
  return true;
}


template<typename T> inline
static bool send_value(BaseChannel *channelp, const T &value,
                       systime_t timeout = TIME_INFINITE) {

  return send_chunk_rev(channelp, reinterpret_cast<const void *>(&value),
                        sizeof(T), timeout);
}


template<typename T> inline
static bool recv_value(BaseChannel *channelp, T &value,
                       systime_t timeout = TIME_INFINITE) {

  return recv_chunk_rev(channelp, reinterpret_cast<void *>(&value),
                        sizeof(T), timeout);
}


static bool skip_after_char(BaseChannel *channelp, char c,
                            systime_t timeout = TIME_INFINITE) {

  register msg_t value;
  do {
    value = chnGetTimeout(channelp, timeout);
    if ((value & ~0xFF) != 0) return false;
  } while (static_cast<char>(value) != c);
  return true;
}


static bool send_msg(BaseChannel *channelp, const Message &msg,
                     size_t msg_size, const char *topicp,
                     const Time &deadline) {

  // Send the deadline
  if (!send_char(channelp, '@')) return false;
  if (!send_value(channelp, deadline.raw)) return false;
  if (!send_char(channelp, ':')) return false;

  // Send the topic name
  uint8_t namelen = ::strlen(topicp);
  if (!send_value(channelp, namelen)) return false;
  if (!send_string(channelp, topicp, namelen)) return false;

  // Send the payload length and data
  if (!send_char(channelp, ':')) return false;
  if (!send_value(channelp, msg_size)) return false;
  if (!send_chunk(channelp, msg.get_raw_data(), msg_size)) return false;

  if (!send_char(channelp, '\r')) return false;
  if (!send_char(channelp, '\n')) return false;
  return true;
}


static bool send_pubsub_msg(BaseChannel *channelp, const Topic &topic,
                            size_t queue_length = 0) {

  R2P_ASSERT(queue_length <= 255);

  // Send the deadline
  if (!send_char(channelp, '@')) return false;
  if (!send_value(channelp, Time::now().raw)) return false;
  if (!send_char(channelp, ':')) return false;

  // Send the management topic name (null string)
  if (!send_value(channelp, static_cast<uint8_t>(0))) return false;
  if (!send_char(channelp, ':')) return false;

  // Discriminate between publisher and subscriber
  if (queue_length == 0) {
    if (!send_char(channelp, 'p')) return false;
  } else {
    if (!send_char(channelp, 's')) return false;
    uint8_t length = static_cast<uint8_t>(queue_length);
    if (!send_value(channelp, length)) return false;
  }
  if (!send_char(channelp, ':')) return false;

  // Send the module and topic names
  const char *namep = Middleware::instance.get_module_name();
  uint8_t namelen = static_cast<uint8_t>(::strlen(namep));
  if (!send_value(channelp, namelen)) return false;
  if (!send_string(channelp, namep, namelen)) return false;
  if (!send_char(channelp, ':')) return false;
  namep = topic.get_name();
  namelen = static_cast<uint8_t>(::strlen(namep));
  if (!send_value(channelp, namelen)) return false;
  if (!send_string(channelp, namep, namelen)) return false;

  if (!send_char(channelp, '\r')) return false;
  if (!send_char(channelp, '\n')) return false;
  return true;
}


static bool send_stop_msg(BaseChannel *channelp) {

  // Send the deadline
  if (!send_char(channelp, '@')) return false;
  if (!send_value(channelp, Time::now().raw)) return false;
  if (!send_char(channelp, ':')) return false;

  // Send the management topic name (null string)
  if (!send_value(channelp, static_cast<uint8_t>(0))) return false;
  if (!send_char(channelp, ':')) return false;

  // Stop command
  if (!send_char(channelp, 't')) return false;

  if (!send_char(channelp, '\r')) return false;
  if (!send_char(channelp, '\n')) return false;
  return true;
}


static bool send_reboot_msg(BaseChannel *channelp) {

  // Send the deadline
  if (!send_char(channelp, '@')) return false;
  if (!send_value(channelp, Time::now().raw)) return false;
  if (!send_char(channelp, ':')) return false;

  // Send the management topic name (null string)
  if (!send_value(channelp, static_cast<uint8_t>(0))) return false;
  if (!send_char(channelp, ':')) return false;

  // Reboot command
  if (!send_char(channelp, 'r')) return false;

  if (!send_char(channelp, '\r')) return false;
  if (!send_char(channelp, '\n')) return false;
  return true;
}


bool DebugTransport::send_advertisement(const Topic &topic) {

  send_lock.acquire();
  bool success = send_pubsub_msg(channelp, topic);
  send_lock.release();
  return success;
}


bool DebugTransport::send_subscription(const Topic &topic,
                                       size_t queue_length) {

  send_lock.acquire();
  bool success = send_pubsub_msg(channelp, topic, queue_length);
  send_lock.release();
  return success;
}


bool DebugTransport::send_stop() {

  send_lock.acquire();
  bool success = send_stop_msg(channelp);
  send_lock.release();
  return success;
}


bool DebugTransport::send_reboot() {

  send_lock.acquire();
  bool success = send_reboot_msg(channelp);
  send_lock.release();
  return success;
}


void DebugTransport::initialize(void *rx_stackp, size_t rx_stacklen,
                                Thread::Priority rx_priority,
                                void *tx_stackp, size_t tx_stacklen,
                                Thread::Priority tx_priority) {

  tx_threadp = Thread::create_static(tx_stackp, tx_stacklen, tx_priority,
                                     tx_threadf, this, "DEBUG_TX");
  R2P_ASSERT(tx_threadp != NULL);

  rx_threadp = Thread::create_static(rx_stackp, rx_stacklen, rx_priority,
                                     rx_threadf, this, "DEBUG_RX" );
  R2P_ASSERT(rx_threadp != NULL);

  advertise(mgmt_rpub, "R2P", Time::INFINITE, sizeof(MgmtMsg));
  subscribe(mgmt_rsub, "R2P", mgmt_msgbuf, MGMT_BUFFER_LENGTH,
            sizeof(MgmtMsg));

  Middleware::instance.add(*this);
}

bool DebugTransport::spin_tx() {

  Message *msgp;
  Time timestamp;
  const BaseSubscriberQueue::Link *subp_linkp;

  SysLock::acquire();
  subp_sem.wait_unsafe();
  subp_queue.peek_unsafe(subp_linkp);
  R2P_ASSERT(subp_linkp->itemp != NULL);
  DebugSubscriber &sub = static_cast<DebugSubscriber &>(*subp_linkp->itemp);
  size_t queue_length = sub.get_queue_count();
  R2P_ASSERT(queue_length > 0);
  SysLock::release();

  while (queue_length-- > 0) {
    SysLock::acquire();
    sub.fetch_unsafe(msgp, timestamp);
    if (queue_length == 0) {
      subp_queue.skip_unsafe();
      if (sub.get_queue_count() > 0) {
        notify_first_sub_unsafe(sub);
      }
    }
    SysLock::release();

    const Topic &topic = *sub.get_topic();
    send_lock.acquire();
    if (!send_msg(channelp, *msgp, topic.get_size(), topic.get_name(),
                  timestamp + topic.get_publish_timeout())) {
      send_char(channelp, '\r');
      while (!send_char(channelp, '\n')) {
        Thread::yield();
      }
      send_lock.release();
      sub.release(*msgp);
      return false;
    }
    send_lock.release();
    sub.release(*msgp);
  }
  return true;
}


bool DebugTransport::spin_rx() {

  // Skip the deadline
  if (!skip_after_char(channelp, '@')) return false;
  { Time deadline; if (!recv_value(channelp, deadline.raw)) return false; }

  // Receive the topic name length and data
  if (!expect_char(channelp, ':')) return false;
  uint8_t namelen;
  if (!recv_value(channelp, namelen)) return false;
  if (namelen > NamingTraits<Topic>::MAX_LENGTH) return false;
  if (!recv_string(channelp, namebufp, namelen)) return false;
  namebufp[namelen] = 0;
  if (!expect_char(channelp, ':')) return false;

  // Check if it is a management message
  if (namelen > 0) { // Normal message
    // Check if the topic is known
    Topic *topicp = Middleware::instance.find_topic(namebufp);
    if (topicp == NULL) return false;
    const StaticList<RemotePublisher>::Link *linkp =
      publishers.find_first(BasePublisher::has_topic, topicp->get_name());
    if (linkp == NULL) return false;

    // Get the payload length
    Message::LengthType datalen;
    if (!recv_value(channelp, datalen)) return false;
    if (datalen != topicp->get_size()) {
      return false;
    }

    // Allocate a new message
    Message *msgp;
    if (!linkp->itemp->alloc(msgp)) return false;

    // Get the payload data
    if (!recv_chunk(channelp, const_cast<uint8_t *>(msgp->get_raw_data()),
                    datalen)) {
      topicp->free(*msgp);
      return false;
    }

    // Forward the message locally
    return linkp->itemp->publish_locally(*msgp);
  }
  else { // Management message
    // Read the management message type character
    char typechar;
    if (!recv_char(channelp, typechar)) return false;
    switch (typechar) {
    case 'p': case 'P':
    case 's': case 'S': {
      // Ignore the module name
      if (!expect_char(channelp, ':')) return false;
      if (!recv_value(channelp, namelen)) return false;
      if (namelen < 1 || namelen > NamingTraits<Middleware>::MAX_LENGTH) {
        return false;
      }
      if (!recv_string(channelp, namebufp, namelen)) return false;

      // Get the topic
      if (!expect_char(channelp, ':')) return false;
      if (!recv_value(channelp, namelen)) return false;
      if (namelen < 1 || namelen > NamingTraits<Topic>::MAX_LENGTH) {
        return false;
      }
      if (!recv_string(channelp, namebufp, namelen)) return false;

      if (typechar == 'p' || typechar == 'P') {
        MgmtMsg *msgp;
        if (mgmt_rpub.alloc(reinterpret_cast<Message *&>(msgp))) {
          msgp->type = MgmtMsg::CMD_ADVERTISE;
          ::strncpy(msgp->pubsub.topic, namebufp,
                    NamingTraits<Topic>::MAX_LENGTH);
          msgp->pubsub.transportp = this;
          return mgmt_rpub.publish_locally(*msgp);
        }
        return false;
      } else {
        // Get the queue length
        uint8_t queue_length;
        if (!recv_value(channelp, queue_length)) return false;
        if (queue_length == 0) return false;
        MgmtMsg *msgp;
        if (mgmt_rpub.alloc(reinterpret_cast<Message *&>(msgp))) {
          msgp->type = MgmtMsg::CMD_SUBSCRIBE;
          ::strncpy(msgp->pubsub.topic, namebufp,
                    NamingTraits<Topic>::MAX_LENGTH);
          msgp->pubsub.transportp = this;
          msgp->pubsub.queue_length = queue_length;
          return mgmt_rpub.publish_locally(*msgp);
        }
        return false;
      }
      return true;
    }
    case 't': case 'T': {
      Middleware::instance.stop();
      return true;
    }
    case 'r': case 'R': {
      Middleware::instance.reboot();
      return true;
    }
    default: return false;
    }
  }
}


Thread::Return DebugTransport::rx_threadf(Thread::Argument arg) {

  R2P_ASSERT(arg != static_cast<Thread::Argument>(NULL));

  for (;;) {
    reinterpret_cast<DebugTransport *>(arg)->spin_rx();
  }
  return static_cast<Thread::Return>(0);
}


Thread::Return DebugTransport::tx_threadf(Thread::Argument arg) {

  R2P_ASSERT(arg != static_cast<Thread::Argument>(NULL));

  for (;;) {
    reinterpret_cast<DebugTransport *>(arg)->spin_tx();
  }
  return static_cast<Thread::Return>(0);
}


DebugTransport::DebugTransport(BaseChannel *channelp, char namebuf[])
:
  Transport(),
  rx_threadp(NULL),
  tx_threadp(NULL),
  channelp(channelp),
  namebufp(namebuf),
  mgmt_rsub(*this, mgmt_msgqueue_buf, MGMT_BUFFER_LENGTH),
  mgmt_rpub()
{
  R2P_ASSERT(channelp != NULL);
  R2P_ASSERT(namebuf != NULL);
}


} // namespace r2p
