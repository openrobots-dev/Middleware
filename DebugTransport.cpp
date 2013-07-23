
#include "DebugTransport.hpp"
#include <r2p/Middleware.hpp>

#include <cstring>

namespace r2p {


static bool send_char(BaseChannel *channelp, char c,
                      systime_t timeout = TIME_INFINITE) {

  return chnPutTimeout(channelp, c, timeout) == Q_OK;
}


static bool expect_char(BaseChannel *channelp, char c,
                        systime_t timeout = TIME_INFINITE) {

  return chnGetTimeout(channelp, timeout) == c;
}


static bool recv_char(BaseChannel *channelp, char &c,
                      systime_t timeout = TIME_INFINITE) {

  register msg_t value = chnGetTimeout(channelp, timeout);
  if (value >= 0 && value <= 255) {
    c = static_cast<char>(value);
    return true;
  }
  return false;
}


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

  const uint8_t *p = reinterpret_cast<const uint8_t *>(chunkp) + length - 1;
  while (length-- > 0) {
    if (!send_byte(channelp, *p--, timeout)) return false;
  }
  return true;
}


static bool recv_chunk_rev(BaseChannel *channelp,
                          void *chunkp, size_t length,
                          systime_t timeout = TIME_INFINITE) {

  uint8_t *p = reinterpret_cast<uint8_t *>(chunkp) + length - 1;
  while (length-- > 0) {
    if (!recv_byte(channelp, *p--, timeout)) return false;
  }
  return true;
}


static bool send_string(BaseChannel *channelp,
                        const char *stringp, size_t length,
                        systime_t timeout = TIME_INFINITE) {

  while (length-- > 0) {
    if (!send_char(channelp, *stringp++, timeout)) return false;
  }
  return true;
}


static bool recv_string(BaseChannel *channelp,
                        char *stringp, size_t length,
                        systime_t timeout = TIME_INFINITE) {

  while (length-- > 0) {
    if (!recv_char(channelp, *stringp++, timeout)) return false;
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
    if (value < 0 || value > 255) return false;
  } while (static_cast<char>(value) != c);
  return true;
}


static bool send_msg(BaseChannel *channelp, const BaseMessage &msg,
                     size_t msg_size, const char *topicp,
                     const Time &deadline) {

  // Send the deadline
  if (!send_char(channelp, '@')) return false;
  if (!send_value(channelp, deadline.raw)) return false;

  // Send the topic name
  if (!send_char(channelp, ':')) return false;
  uint8_t namelen = ::strlen(topicp);
  if (!send_value(channelp, namelen)) return false;
  if (!send_string(channelp, topicp, namelen)) return false;

  // Send the payload length and data
  if (!send_char(channelp, ':')) return false;
  msg_size -= sizeof(BaseMessage::RefcountType);
  BaseMessage::LengthType msg_length =
    static_cast<BaseMessage::LengthType>(msg_size);
  if (!send_value(channelp, msg_length)) return false;
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
  if (!send_value(channelp, Time::Type(0))) return false;

  // Send the management topic name (null string)
  if (!send_char(channelp, ':')) return false;
  if (!send_value(channelp, uint8_t(0))) return false;
  if (!send_char(channelp, ':')) return false;

  // Discriminate between publisher and subscriber
  if (queue_length == 0) {
    if (!send_char(channelp, 'p')) return false;
  } else {
    if (!send_char(channelp, 's')) return false;
    uint8_t length = static_cast<uint8_t>(queue_length);
    if (!send_value(channelp, length)) return false;
  }

  // Send the module and topic names
  const char *namep = Middleware::instance.get_name();
  uint8_t namelen = static_cast<uint8_t>(::strlen(namep));
  if (!send_value(channelp, namelen)) return false;
  if (!send_string(channelp, namep, namelen)) return false;
  if (!send_char(channelp, ':')) return false;
  namep = topic.get_name();
  namelen = static_cast<uint8_t>(::strlen(namep));
  if (!send_value(channelp, namelen)) return false;
  if (!send_string(channelp, namep, namelen)) return false;
  if (!send_char(channelp, ':')) return false;

  if (!send_char(channelp, '\r')) return false;
  if (!send_char(channelp, '\n')) return false;
  return true;
}


static bool send_stop_msg(BaseChannel *channelp) {

  // Send the deadline
  if (!send_char(channelp, '@')) return false;
  if (!send_value(channelp, Time::Type(0))) return false;

  // Send the management topic name (null string)
  if (!send_char(channelp, ':')) return false;
  if (!send_value(channelp, uint8_t(0))) return false;
  if (!send_char(channelp, ':')) return false;
  if (!send_char(channelp, 't')) return false;

  if (!send_char(channelp, '\r')) return false;
  if (!send_char(channelp, '\n')) return false;
  return true;
}


static bool send_reboot_msg(BaseChannel *channelp) {

  // Send the deadline
  if (!send_char(channelp, '@')) return false;
  if (!send_value(channelp, Time::Type(0))) return false;

  // Send the management topic name (null string)
  if (!send_char(channelp, ':')) return false;
  if (!send_value(channelp, uint8_t(0))) return false;
  if (!send_char(channelp, ':')) return false;
  if (!send_char(channelp, 'r')) return false;

  if (!send_char(channelp, '\r')) return false;
  if (!send_char(channelp, '\n')) return false;
  return true;
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

  Middleware &mw = Middleware::instance;

  mw.advertise(info_rpub, "R2P_INFO", Time::INFINITE, sizeof(InfoMsg));
  publishers.link(info_rpub.by_transport);

  mw.subscribe_remote(info_rsub, "R2P_INFO", info_msgbuf, INFO_BUFFER_LENGTH,
                      sizeof(InfoMsg));
  subscribers.link(info_rsub.by_transport.entry);

  mw.add(*this);
}


bool DebugTransport::spin_tx() {

  BaseMessage *msgp;
  Time timestamp;
  const BaseSubscriberQueue::Link *subp_linkp;

  SysLock::acquire();
  subp_sem.wait_unsafe();
  subp_queue.peek_unsafe(subp_linkp);
  R2P_ASSERT(subp_linkp->datap != NULL);
  DebugSubscriber &sub = static_cast<DebugSubscriber &>(*subp_linkp->datap);
  size_t queue_length = sub.get_queue_count_unsafe();
  R2P_ASSERT(queue_length > 0);
  SysLock::release();

  while (queue_length-- > 0) {
    SysLock::acquire();
    sub.fetch(msgp, timestamp);
    if (queue_length == 0) {
      subp_queue.skip_unsafe();
      if (sub.get_queue_count_unsafe() > 0) {
        notify_first_sub_unsafe(sub);
      }
    }
    SysLock::release();

    const Topic &topic = *sub.get_topic();
    send_lock.acquire();
    if (!send_msg(channelp, *msgp, topic.get_msg_size(), topic.get_name(),
                  timestamp + topic.get_publish_timeout())) {
      send_char(channelp, '\r');
      while (!send_char(channelp, '\n')) {
        chThdYield();
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
  Time deadline(0);
  if (!recv_value(channelp, deadline.raw)) return false;

  // Receive the topic name length and data
  if (!expect_char(channelp, ':')) return false;
  uint8_t namelen;
  if (!recv_value(channelp, namelen)) return false;
  if (namelen > R2P_TOPIC_MAXNAMELEN) return false;
  if (!recv_string(channelp, namebufp, namelen)) return false;
  namebufp[namelen] = 0;
  if (!expect_char(channelp, ':')) return false;

  // Check if it is a management message
  if (namelen > 0) {
    // Check if the topic is known
    Topic *topicp = Middleware::instance.find_topic(namebufp);
    if (topicp == NULL) return false;

    // Get the payload length
    BaseMessage::LengthType datalen;
    if (!recv_value(channelp, datalen)) return false;
    if (datalen + sizeof(BaseMessage::RefcountType) !=
        topicp->get_msg_size()) {
      return false;
    }
    R2P_ASSERT(datalen + sizeof(BaseMessage::RefcountType) <= namebuflen);

    // Allocate a new message
    const StaticList<DebugPublisher>::Link *linkp =
      publishers.find_first(DebugPublisher::has_topic, topicp->get_name());
    if (linkp == NULL) return false;
    BaseMessage *msgp;
    if (!linkp->datap->alloc(msgp)) return false;

    // Get the payload data
    if (!recv_chunk(channelp, const_cast<uint8_t *>(msgp->get_raw_data()),
                    datalen)) {
      topicp->free(*msgp);
      return false;
    }

    // Forward the message locally
    return linkp->datap->publish_locally(*msgp);
  } else {
    // Management message

    // Read the management message type character
    char typechar;
    if (!recv_char(channelp, typechar)) return false;
    switch (typechar) {
    case 'p': case 'P':
    case 's': case 'S': {
      // Read the module name
      if (!recv_value(channelp, namelen)) return false;
      if (namelen < 1 || namelen > R2P_TOPIC_MAXNAMELEN) return false;
      if (!recv_string(channelp, namebufp, namelen)) return false;
      namebufp[namelen] = 0;

      // Read the topic name
      if (!expect_char(channelp, ':')) return false;
      if (!recv_value(channelp, namelen)) return false;
      if (namelen < 1 || namelen > R2P_TOPIC_MAXNAMELEN) return false;
      if (!recv_string(channelp, namebufp, namelen)) return false;
      namebufp[namelen] = 0;
      Topic *topicp = Middleware::instance.find_topic(namebufp);

      if (typechar == 'p' || typechar == 'P') {
        // Process only if there are local publishers
        if (topicp == NULL) return true;

        // Check if the remote publisher already exists
        const StaticList<DebugPublisher>::Link *linkp;
        linkp = publishers.find_first(DebugPublisher::has_topic, namebufp);
        if (linkp == NULL) {
          // Create a new remote publisher
          DebugPublisher *pubp = new DebugPublisher();
          if (pubp == NULL) return false;
          pubp->advertise_cb(*topicp);
          topicp->advertise_cb(*pubp, Time::INFINITE);
          publishers.link(pubp->by_transport);
        }
        return true;
      } else if (typechar == 's' || typechar == 'S') {
        // Get the queue length
        uint8_t queue_length;
        if (!recv_value(channelp, queue_length)) return false;
        if (queue_length == 0) return false;

        // Process only if there are local subscribers
        if (topicp == NULL) return true;

        // Check if the remote subscriber already exists
        const StaticList<DebugSubscriber>::Link *linkp;
        linkp = subscribers.find_first(DebugSubscriber::has_topic, namebufp);
        if (linkp == NULL) {
          // Create a new remote subscriber
          BaseMessage *msgpool_bufp = NULL;
          TimestampedMsgPtrQueue::Entry *queue_bufp = NULL;
          DebugSubscriber *subp = NULL;

          msgpool_bufp = reinterpret_cast<BaseMessage *>(
            new uint8_t[topicp->get_msg_size() * queue_length]
          );
          if (msgpool_bufp != NULL) {
            queue_bufp = new TimestampedMsgPtrQueue::Entry[queue_length];
            if (queue_bufp != NULL) {
              subp = new DebugSubscriber(queue_bufp, queue_length, *this);
              if (subp != NULL) {
                subp->subscribe_cb(*topicp);
                topicp->extend_pool(msgpool_bufp, queue_length);
                topicp->subscribe_remote_cb(*subp);
                subscribers.link(subp->by_transport.entry);
              }
            }
          }
          if (msgpool_bufp == NULL || queue_bufp == NULL || subp == NULL) {
            delete subp;
            delete [] queue_bufp;
            delete [] msgpool_bufp;
          }
        }
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


void DebugTransport::notify_advertise(Topic &topic) {

  send_lock.acquire();
  send_pubsub_msg(channelp, topic);
  send_lock.release();
}


void DebugTransport::notify_subscribe(Topic &topic, size_t queue_length) {

  R2P_ASSERT(queue_length > 0);

  send_lock.acquire();
  send_pubsub_msg(channelp, topic, queue_length);
  send_lock.release();
}


void DebugTransport::notify_stop() {

  send_lock.acquire();
  while (!send_stop_msg(channelp)) {}
  send_lock.release();
}


void DebugTransport::notify_reboot() {

  send_lock.acquire();
  while (!send_reboot_msg(channelp)) {}
  send_lock.release();
}


Thread::ReturnType DebugTransport::rx_threadf(Thread::ArgumentType arg) {

  R2P_ASSERT(arg != static_cast<Thread::ArgumentType>(0));

  for (;;) {
    reinterpret_cast<DebugTransport *>(arg)->spin_rx();
  }
  return 0;
}


Thread::ReturnType DebugTransport::tx_threadf(Thread::ArgumentType arg) {

  R2P_ASSERT(arg != static_cast<Thread::ArgumentType>(0));

  for (;;) {
    reinterpret_cast<DebugTransport *>(arg)->spin_tx();
  }
  return 0;
}


DebugTransport::DebugTransport(
  const char *namep, BaseChannel *channelp,
  char namebuf[], size_t namebuflen)
:
  BaseTransport(namep),
  rx_threadp(NULL),
  tx_threadp(NULL),
  channelp(channelp),
  namebufp(namebuf),
  namebuflen(namebuflen),
  info_rsub(info_msgqueue_buf, INFO_BUFFER_LENGTH, *this),
  info_rpub()
{
  R2P_ASSERT(channelp != NULL);
  R2P_ASSERT(namebuf != NULL);
  R2P_ASSERT(namebuflen >= sizeof(BaseMessage));
}


} // namespace r2p
