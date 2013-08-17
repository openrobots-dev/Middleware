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

public:
  DebugTransport(BaseChannel *channelp, char namebuf[]);

private:
  static Thread::Return rx_threadf(Thread::Argument arg);
  static Thread::Return tx_threadf(Thread::Argument arg);
};


inline
void DebugTransport::notify_first_sub_unsafe(const DebugSubscriber &sub) {

  subp_queue.post_unsafe(sub.by_transport_notify);
  subp_sem.signal_unsafe();
}


} // namespace r2p
