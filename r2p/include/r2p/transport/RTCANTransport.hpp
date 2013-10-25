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
public:

public:
  struct adv_msg_t {
    char type;
    uint8_t queue_length;
    uint16_t payload_size;
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

  enum { MGMT_BUFFER_LENGTH = 2 };
  MgmtMsg mgmt_msgbuf[MGMT_BUFFER_LENGTH];
  TimestampedMsgPtrQueue::Entry mgmt_msgqueue_buf[MGMT_BUFFER_LENGTH];
  RTCANSubscriber mgmt_rsub;
  RTCANPublisher mgmt_rpub;

public:
  bool send_advertisement(const Topic &topic);
  bool send_subscription_request(const Topic &topic);
  bool send_subscription_response(const Topic &topic);
  bool send_stop();
  bool send_reboot();
  bool send_bootload();
  bool send(Message * msgp, RTCANSubscriber * rsubp);

  rtcan_id_t topic_id(const char * namep) const; // FIXME

  void initialize(const RTCANConfig &rtcan_config);

private:
  bool send_adv_msg(const adv_msg_t &adv_msg);
  void recv_adv_msg(const adv_msg_t &adv_msg);
  RemotePublisher *create_publisher(Topic &topic, const uint8_t *raw_params = NULL) const;
  RemoteSubscriber *create_subscriber(
    Topic &topic,
    TimestampedMsgPtrQueue::Entry queue_buf[], // TODO: remove
    size_t queue_length,
    const uint8_t *raw_params = NULL
  ) const;

public:
  RTCANTransport(RTCANDriver &rtcan);
  ~RTCANTransport();

private:
  static void adv_rx_cb(rtcan_msg_t &rtcan_msg);
  static void send_cb(rtcan_msg_t &rtcan_msg);
  static void recv_cb(rtcan_msg_t &rtcan_msg);
  static void free_header(rtcan_msg_t &rtcan_msg);
};

} // namespace r2p
