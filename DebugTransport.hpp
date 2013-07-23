
#ifndef __R2P__DEBUGTRANSPORT_HPP__
#define __R2P__DEBUGTRANSPORT_HPP__

#include <r2p/common.hpp>
#include <r2p/BaseTransport.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/Mutex.hpp>
#include <r2p/InfoMsg.hpp>
#include <r2p/Semaphore.hpp>

#include <ch.h>
#include <io_channel.h>

#include "DebugPublisher.hpp"
#include "DebugSubscriber.hpp"
#include <r2p/Thread.hpp>

namespace r2p {

class BaseMessage;


class DebugTransport : public BaseTransport {
private:
  Thread *rx_threadp;
  Thread *tx_threadp;

  BaseChannel *channelp;
  char *namebufp;
  size_t namebuflen;

  StaticList<DebugPublisher> publishers;
  StaticList<DebugSubscriber> subscribers;

  Semaphore subp_sem;
  BaseSubscriberQueue subp_queue;

  Mutex send_lock;

  enum { INFO_BUFFER_LENGTH = 2 };
  InfoMsg info_msgbuf[INFO_BUFFER_LENGTH];
  TimestampedMsgPtrQueue::Entry info_msgqueue_buf[INFO_BUFFER_LENGTH];
  DebugSubscriber info_rsub;
  DebugPublisher info_rpub;

public:
  void initialize(void *rx_stackp, size_t rx_stacklen,
                  Thread::Priority rx_priority,
                  void *tx_stackp, size_t tx_stacklen,
                  Thread::Priority tx_priority);

  void notify_first_sub_unsafe(DebugSubscriber &sub);

  void notify_advertise(Topic &topic);
  void notify_subscribe(Topic &topic, size_t queue_length);
  void notify_stop();
  void notify_reboot();

private:
  bool spin_tx();
  bool spin_rx();

private:
  static Thread::ReturnType rx_threadf(Thread::ArgumentType arg);
  static Thread::ReturnType tx_threadf(Thread::ArgumentType arg);

public:
  DebugTransport(const char *namep, BaseChannel *channelp,
                 char namebuf[], size_t namebuflen);
};


inline
void DebugTransport::notify_first_sub_unsafe(DebugSubscriber &sub) {

  subp_queue.post_unsafe(sub.by_transport_notify.entry);
  subp_sem.signal_unsafe();
}


} // namespace r2p
#endif // __R2P__DEBUGTRANSPORT_HPP__
