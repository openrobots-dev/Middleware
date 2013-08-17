#include <cstring>

#include "RTCANTransport.hpp"
#include "RTCANPublisher.hpp"
#include "RTCANSubscriber.hpp"
#include <r2p/Middleware.hpp>

#include "rtcan.h"

//FIXME needed for header pool, should use abstract allocator
#include "ch.h"

namespace r2p {

void RTCANTransport::adv_rx_cb(rtcan_msg_t &rtcan_msg) {
//  RTCANTransport &transport = reinterpret_cast<RTCANTransport &>(rtcan_msg.params);
//  transport.recv_adv_msg(reinterpret_cast<const adv_msg_t &>(rtcan_msg.data));

  RTCANTransport * transport = (RTCANTransport *)rtcan_msg.params;
  transport->recv_adv_msg(*(const adv_msg_t *)(rtcan_msg.data));
}



void RTCANTransport::recv_adv_msg(const adv_msg_t &adv_msg) {
  MgmtMsg *msgp;

  switch (adv_msg.type) {
  case 'P':
    if (mgmt_rpub.alloc_unsafe(reinterpret_cast<Message *&>(msgp))) {
	  msgp->type = MgmtMsg::CMD_ADVERTISE;
	  ::strncpy(msgp->pubsub.topic, adv_msg.topic,
				NamingTraits<Topic>::MAX_LENGTH);
	  msgp->pubsub.transportp = this;
	  mgmt_rpub.publish_locally_unsafe(*msgp);
	}
	break;
  case 'S':
      if (mgmt_rpub.alloc_unsafe(reinterpret_cast<Message *&>(msgp))) {
        msgp->type = MgmtMsg::CMD_SUBSCRIBE;
        ::strncpy(msgp->pubsub.topic, adv_msg.topic,
                  NamingTraits<Topic>::MAX_LENGTH);
        msgp->pubsub.transportp = this;
        msgp->pubsub.queue_length = static_cast<size_t>(adv_msg.queue_length);
        mgmt_rpub.publish_locally_unsafe(*msgp);
      }
	  break;
  default:
	  R2P_ASSERT(false && "Should never happen");
	  break;
  }
}

bool RTCANTransport::send_adv_msg(const adv_msg_t &adv_msg) {
  rtcan_msg_t * rtcan_msg_p;

  // Allocate RTCAN msg header
  rtcan_msg_p = header_pool.alloc();
  if (rtcan_msg_p == NULL) return false;

  rtcan_msg_p->id = 123 << 8;
  // FIXME need callback to free the message header
  rtcan_msg_p->callback = NULL;
  rtcan_msg_p->params = NULL;
  rtcan_msg_p->size = sizeof(adv_msg_t);
  rtcan_msg_p->status = RTCAN_MSG_READY;
  rtcan_msg_p->data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(&adv_msg));

  rtcanTransmit(&rtcan, rtcan_msg_p, 100);

  return true;
}

bool RTCANTransport::send_advertisement(const Topic &topic) {

  adv_tx_msg.type = 'P';
  adv_tx_msg.queue_length = 0;
  ::strncpy(adv_tx_msg.topic, topic.get_name(), NamingTraits<Topic>::MAX_LENGTH);
  adv_tx_msg.rtcan_id = 123;

  return send_adv_msg(adv_tx_msg);
}


bool RTCANTransport::send_subscription(const Topic &topic,
                                       size_t queue_length) {

  R2P_ASSERT(queue_length <= 255);

  adv_tx_msg.type = 'S';
  adv_tx_msg.queue_length = queue_length;
  ::strncpy(adv_tx_msg.topic, topic.get_name(), NamingTraits<Topic>::MAX_LENGTH);
  adv_tx_msg.rtcan_id = 123;

  return send_adv_msg(adv_tx_msg);
}


bool RTCANTransport::send_stop() {

  // TODO
  return true;
}


bool RTCANTransport::send_reboot() {

  // TODO
  return true;
}

bool RTCANTransport::send(Message * msgp, RTCANSubscriber * rsubp) {
	rtcan_msg_t * rtcan_msg_p;

	// Allocate RTCAN msg header
	rtcan_msg_p = header_pool.alloc_unsafe();
	if (rtcan_msg_p == NULL) return false;

	rtcan_msg_p->id = rsubp->rtcan_id;
//FIXME XXX
	rtcan_msg_p->callback = reinterpret_cast<rtcan_msgcallback_t>(send_cb);
	rtcan_msg_p->params = rsubp;
	rtcan_msg_p->params2 = msgp;
	// FIXME
	rtcan_msg_p->size = rsubp->get_topic()->get_size();
	rtcan_msg_p->status = RTCAN_MSG_READY;
	rtcan_msg_p->data = (uint8_t *)msgp->get_raw_data();

	rtcanTransmitI(&rtcan, rtcan_msg_p, 100);
  // TODO
  return true;
}

#include "hal.h"
void RTCANTransport::send_cb(rtcan_msg_t &rtcan_msg) {
//  RTCANTransport &transport = reinterpret_cast<RTCANTransport &>(rtcan_msg.params);
//  transport.recv_adv_msg(reinterpret_cast<const adv_msg_t &>(rtcan_msg.data));

  RTCANSubscriber * rsubp = (RTCANSubscriber *)rtcan_msg.params;
  Message * msg = (Message *)rtcan_msg.params2;

  rsubp->release_unsafe(*msg);
  rsubp->queue_free++;

  palTogglePad(LED_GPIO, LED3);

}







void RTCANTransport::initialize(const RTCANConfig &rtcan_config) {


  rtcanInit();
  rtcanStart(&rtcan, &rtcan_config);

  rtcanReceive(&rtcan, &adv_rx_header);

  advertise(mgmt_rpub, "R2P", Time::INFINITE, sizeof(MgmtMsg));
  subscribe(mgmt_rsub, "R2P", mgmt_msgbuf, MGMT_BUFFER_LENGTH,
            sizeof(MgmtMsg));

  Middleware::instance.add(*this);
}


RemotePublisher *RTCANTransport::create_publisher() const {

  return new RTCANPublisher();
}


RemoteSubscriber *RTCANTransport::create_subscriber(
  Transport &transport,
  TimestampedMsgPtrQueue::Entry queue_buf[],
  size_t queue_length) const {

  return new RTCANSubscriber(static_cast<RTCANTransport &>(transport), queue_buf, queue_length);
}

/*
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
  Time deadline(0);
  if (!recv_value(channelp, deadline.raw)) return false;

  // Receive the topic name length and data
  if (!expect_char(channelp, ':')) return false;
  uint8_t namelen;
  if (!recv_value(channelp, namelen)) return false;
  if (namelen > NamingTraits<Topic>::MAX_LENGTH) return false;
  if (!recv_string(channelp, namebufp, namelen)) return false;
  namebufp[namelen] = 0;
  if (!expect_char(channelp, ':')) return false;

  // Check if it is a management message
  if (namelen > 0) {
    // Check if the topic is known
    Topic *topicp = Middleware::instance.find_topic(namebufp);
    if (topicp == NULL) return false;
    const StaticList<RemotePublisher>::Link *linkp =
      publishers.find_first(BasePublisher::has_topic, topicp->get_name());
    if (linkp == NULL) return false;

    // Get the payload length
    BaseMessage::LengthType datalen;
    if (!recv_value(channelp, datalen)) return false;
    if (datalen + sizeof(BaseMessage::RefcountType) != topicp->get_size()) {
      return false;
    }
    R2P_ASSERT(datalen + sizeof(BaseMessage::RefcountType) <= namebuflen);

    // Allocate a new message
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
      if (namelen < 1 || namelen > NamingTraits<Middleware>::MAX_LENGTH) {
        return false;
      }
      if (!recv_string(channelp, namebufp, namelen)) return false;
      namebufp[namelen] = 0;

      // Read the topic name
      if (!expect_char(channelp, ':')) return false;
      if (!recv_value(channelp, namelen)) return false;
      if (namelen < 1 || namelen > NamingTraits<Topic>::MAX_LENGTH) {
        return false;
      }
      if (!recv_string(channelp, namebufp, namelen)) return false;
      namebufp[namelen] = 0;

      if (typechar == 'p' || typechar == 'P') {
        return advertise(namebufp);
      } else if (typechar == 's' || typechar == 'S') {
        // Get the queue length
        uint8_t queue_length;
        if (!recv_value(channelp, queue_length)) return false;
        if (queue_length == 0) return false;

        return subscribe(namebufp, queue_length);
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
*/

RTCANTransport::RTCANTransport(RTCANDriver &rtcan)
:
  Transport(),
  rtcan(rtcan),
  header_pool(header_buffer, 10),
  mgmt_rsub(*this, mgmt_msgqueue_buf, MGMT_BUFFER_LENGTH),
  mgmt_rpub()
{
  adv_rx_header.id = 123 << 8;
  adv_rx_header.data = reinterpret_cast<uint8_t *>(&adv_rx_msg);
  adv_rx_header.size = sizeof(adv_rx_msg);
  adv_rx_header.callback = reinterpret_cast<rtcan_msgcallback_t>(adv_rx_cb);
  adv_rx_header.params = reinterpret_cast<void *>(this);
  adv_rx_header.status = RTCAN_MSG_READY;
}

} // namespace r2p
