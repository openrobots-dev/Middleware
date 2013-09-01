#pragma once

#include <r2p/common.hpp>
#include <r2p/Transport.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/Mutex.hpp>
#include <r2p/MgmtMsg.hpp>
#include <r2p/Semaphore.hpp>

#include <ch.h>
#include <io_channel.h>

#include "DebugPublisher.hpp"
#include "DebugSubscriber.hpp"
#include <r2p/Thread.hpp>

namespace r2p {


class DebugTransport : public Transport {
private:
  Thread *rx_threadp;
  Thread *tx_threadp;

  BaseChannel *channelp;
  char *namebufp;

  Semaphore subp_sem;
  BaseSubscriberQueue subp_queue;

  Mutex send_lock;

  enum { MGMT_BUFFER_LENGTH = 2 };
  MgmtMsg mgmt_msgbuf[MGMT_BUFFER_LENGTH];
  TimestampedMsgPtrQueue::Entry mgmt_msgqueue_buf[MGMT_BUFFER_LENGTH];
  DebugSubscriber mgmt_rsub;
  DebugPublisher mgmt_rpub;

public:
  bool send_advertisement(const Topic &topic);
  bool send_subscription(const Topic &topic, size_t queue_length);
  bool send_stop();
  bool send_reboot();

  void initialize(void *rx_stackp, size_t rx_stacklen,
                  Thread::Priority rx_priority,
                  void *tx_stackp, size_t tx_stacklen,
                  Thread::Priority tx_priority);

  void notify_first_sub_unsafe(const DebugSubscriber &sub);

private:
  RemotePublisher *create_publisher(Topic &topic) const;
  RemoteSubscriber *create_subscriber(
    Transport &transport,
    TimestampedMsgPtrQueue::Entry queue_buf[],
    size_t queue_length
  ) const;

  bool spin_tx();
  bool spin_rx();

private:
  bool send_char(char c, systime_t timeout = TIME_INFINITE);
  bool expect_char(char c, systime_t timeout = TIME_INFINITE);
  bool skip_after_char(char c, systime_t timeout = TIME_INFINITE);
  bool recv_char(char &c, systime_t timeout = TIME_INFINITE);
  bool send_byte(uint8_t byte, systime_t timeout = TIME_INFINITE);
  bool recv_byte(uint8_t &byte, systime_t timeout = TIME_INFINITE);
  bool send_chunk(const void *chunkp, size_t length,
                  systime_t timeout = TIME_INFINITE);
  bool recv_chunk(void *chunkp, size_t length,
                  systime_t timeout = TIME_INFINITE);
  bool send_chunk_rev(const void *chunkp, size_t length,
                      systime_t timeout = TIME_INFINITE);
  bool recv_chunk_rev(void *chunkp, size_t length,
                      systime_t timeout = TIME_INFINITE);
  bool send_string(const char *stringp, size_t length,
                   systime_t timeout = TIME_INFINITE);
  bool recv_string(char *stringp, size_t length,
                   systime_t timeout = TIME_INFINITE);

  template<typename T>
  bool send_value(const T &value, systime_t timeout = TIME_INFINITE);

  template<typename T> inline
  bool recv_value(T &value, systime_t timeout = TIME_INFINITE);
  bool send_msg(const Message &msg, size_t msg_size, const char *topicp,
                const Time &deadline);
  bool send_pubsub_msg(const Topic &topic, size_t queue_length = 0);
  bool send_stop_msg();
  bool send_reboot_msg();

public:
  DebugTransport(BaseChannel *channelp, char namebuf[]);
  ~DebugTransport();

private:
  static Thread::Return rx_threadf(Thread::Argument arg);
  static Thread::Return tx_threadf(Thread::Argument arg);
};


inline
void DebugTransport::notify_first_sub_unsafe(const DebugSubscriber &sub) {

  subp_queue.post_unsafe(sub.by_transport_notify);
  subp_sem.signal_unsafe();
}


inline
RemotePublisher *DebugTransport::create_publisher(Topic &topic) const {

  (void)topic;
  return new DebugPublisher();
}


inline
RemoteSubscriber *DebugTransport::create_subscriber(
  Transport &transport,
  TimestampedMsgPtrQueue::Entry queue_buf[],
  size_t queue_length) const {

  return new DebugSubscriber(static_cast<DebugTransport &>(transport),
                             queue_buf, queue_length);
}


inline
bool DebugTransport::send_char(char c, systime_t timeout) {

  return chnPutTimeout(channelp, c, timeout) == Q_OK;
}


inline
bool DebugTransport::expect_char(char c, systime_t timeout) {

  return chnGetTimeout(channelp, timeout) == c;
}


inline
bool DebugTransport::recv_char(char &c, systime_t timeout) {

  register msg_t value = chnGetTimeout(channelp, timeout);
  if ((value & ~0xFF) == 0) {
    c = static_cast<char>(value);
    return true;
  }
  return false;
}


inline
bool DebugTransport::send_byte(uint8_t byte, systime_t timeout) {

  static const char hex[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
  };

  if (chnPutTimeout(channelp, hex[byte >> 4], timeout) != Q_OK) return false;
  if (chnPutTimeout(channelp, hex[byte & 15], timeout) != Q_OK) return false;
  return true;
}


template<typename T> inline
bool DebugTransport::send_value(const T &value, systime_t timeout) {

  return send_chunk_rev(reinterpret_cast<const void *>(&value),
                        sizeof(T), timeout);
}


template<> inline
bool DebugTransport::send_value(const uint8_t &value, systime_t timeout) {

  return send_byte(value, timeout);
}


template<> inline
bool DebugTransport::send_value(const char &value, systime_t timeout) {

  return send_byte(static_cast<const uint8_t >(value), timeout);
}


template<typename T> inline
bool DebugTransport::recv_value(T &value, systime_t timeout) {

  return recv_chunk_rev(reinterpret_cast<void *>(&value),
                        sizeof(T), timeout);
}


template<> inline
bool DebugTransport::recv_value(uint8_t &value, systime_t timeout) {

  return recv_byte(value, timeout);
}


template<> inline
bool DebugTransport::recv_value(char &value, systime_t timeout) {

  return recv_byte(reinterpret_cast<uint8_t &>(value), timeout);
}


} // namespace r2p
