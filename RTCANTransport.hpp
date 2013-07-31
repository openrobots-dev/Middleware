#pragma once

#include <r2p/common.hpp>
#include <r2p/BaseTransport.hpp>
#include <r2p/BaseSubscriberQueue.hpp>
#include <r2p/TimestampedMsgPtrQueue.hpp>
#include <r2p/Mutex.hpp>
#include <r2p/InfoMsg.hpp>
#include <r2p/Semaphore.hpp>

#include "rtcan.h"

#include "RTCANPublisher.hpp"
#include "RTCANSubscriber.hpp"
#include <r2p/Thread.hpp>


namespace r2p {

class BaseMessage;

class RTCANTransport : public BaseTransport {
public:

public:
  struct adv_msg_t {
    char type;
    uint8_t queue_length;
    rtcan_id_t rtcan_id;
    char topic[NamingTraits<Topic>::MAX_LENGTH];
  } R2P_PACKED;

private:
  RTCANDriver &rtcan;
  // FIXME to move in pub/sub?
  rtcan_msg_t header_buffer[10];
  MemoryPool<rtcan_msg_t> header_pool;

  rtcan_msg_t adv_rx_header;
  adv_msg_t adv_tx_msg;
  adv_msg_t adv_rx_msg;

  BaseSubscriberQueue subp_queue;

  enum { INFO_BUFFER_LENGTH = 2 };
  InfoMsg info_msgbuf[INFO_BUFFER_LENGTH];
  TimestampedMsgPtrQueue::Entry info_msgqueue_buf[INFO_BUFFER_LENGTH];
  RTCANSubscriber info_rsub;
  RTCANPublisher info_rpub;

public:
  bool send_advertisement(const Topic &topic);
  bool send_subscription(const Topic &topic, size_t queue_length);
  bool send_stop();
  bool send_reboot();

  void initialize(const RTCANConfig &rtcan_config);

private:
  bool send_adv_msg(const adv_msg_t &adv_msg);
  void recv_adv_msg(const adv_msg_t &adv_msg);
  RemotePublisher *create_publisher();
  RemoteSubscriber *create_subscriber(
    BaseTransport &transport,
    TimestampedMsgPtrQueue::Entry queue_buf[], // TODO: remove
    size_t queue_length
  );

public:
  RTCANTransport(const char *namep, RTCANDriver &rtcan);

private:
  static void adv_rx_cb(rtcan_msg_t &rtcan_msg);
};

} // namespace r2p
