#pragma once

#include <r2p/common.hpp>
#include <r2p/Transport.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/Mutex.hpp>
#include <r2p/MgmtMsg.hpp>
#include <r2p/Semaphore.hpp>
#include <r2p/Thread.hpp>
#include <r2p/MemoryPool.hpp>

#include "rtcan.h"
#include "RTCANPublisher.hpp"
#include "RTCANSubscriber.hpp"


namespace r2p {

class Message;

class RTCANTransport : public Transport {
private:
  RTCANDriver &rtcan;
  // FIXME to move in pub/sub?
  rtcan_msg_t header_buffer[10];
  MemoryPool<rtcan_msg_t> header_pool;

  BaseSubscriberQueue subp_queue;

  enum { MGMT_BUFFER_LENGTH = 4 };
  MgmtMsg mgmt_msgbuf[MGMT_BUFFER_LENGTH];
  TimestampedMsgPtrQueue::Entry mgmt_msgqueue_buf[MGMT_BUFFER_LENGTH];
  RTCANSubscriber * mgmt_rsub;
  RTCANPublisher * mgmt_rpub;

public:
  bool send(Message * msgp, RTCANSubscriber * rsubp);

  rtcan_id_t topic_id(const char * namep) const; // FIXME

  void initialize(const RTCANConfig &rtcan_config);

private:
  RemotePublisher *create_publisher(Topic &topic, const uint8_t raw_params[] = NULL) const;
  RemoteSubscriber *create_subscriber(
    Topic &topic,
    TimestampedMsgPtrQueue::Entry queue_buf[], // TODO: remove
    size_t queue_length
  ) const;
  void fill_raw_params(const Topic &topic, uint8_t raw_params[]);

public:
  RTCANTransport(RTCANDriver &rtcan);
  ~RTCANTransport();

private:
  static void send_cb(rtcan_msg_t &rtcan_msg);
  static void recv_cb(rtcan_msg_t &rtcan_msg);
  static void free_header(rtcan_msg_t &rtcan_msg);
};

} // namespace r2p
