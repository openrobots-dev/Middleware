#include <cstring>

#include "r2p/transport/RTCANTransport.hpp"
#include "r2p/transport/RTCANPublisher.hpp"
#include "r2p/transport/RTCANSubscriber.hpp"
#include <r2p/Middleware.hpp>

#include "rtcan.h"

//FIXME needed for header pool, should use abstract allocator
#include "ch.h"

namespace r2p {

void RTCANTransport::adv_rx_cb(rtcan_msg_t &rtcan_msg) {
//  RTCANTransport &transport = reinterpret_cast<RTCANTransport &>(rtcan_msg.params);
//  transport.recv_adv_msg(reinterpret_cast<const adv_msg_t &>(rtcan_msg.data));

	RTCANTransport * transport = (RTCANTransport *) rtcan_msg.params;
	transport->recv_adv_msg(*(const adv_msg_t *) (rtcan_msg.data));
}

void RTCANTransport::recv_adv_msg(const adv_msg_t &adv_msg) {
	MgmtMsg *msgp;

	switch (adv_msg.type) {
	case 'P':
		if (mgmt_rpub.alloc_unsafe(reinterpret_cast<Message *&>(msgp))) {
			msgp->type = MgmtMsg::CMD_ADVERTISE;
			::strncpy(msgp->pubsub.topic, adv_msg.topic, NamingTraits<Topic>::MAX_LENGTH);
			msgp->pubsub.transportp = this;
			mgmt_rpub.publish_locally_unsafe(*msgp);
		}
		break;
	case 'S':
		if (mgmt_rpub.alloc_unsafe(reinterpret_cast<Message *&>(msgp))) {
			msgp->type = MgmtMsg::CMD_SUBSCRIBE_REQUEST;
			::strncpy(msgp->pubsub.topic, adv_msg.topic, NamingTraits<Topic>::MAX_LENGTH);
			msgp->pubsub.transportp = this;
			msgp->pubsub.queue_length = static_cast<size_t>(adv_msg.queue_length);
			mgmt_rpub.publish_locally_unsafe(*msgp);
		}
		break;
	case 'E':
		if (mgmt_rpub.alloc_unsafe(reinterpret_cast<Message *&>(msgp))) {
			msgp->type = MgmtMsg::CMD_SUBSCRIBE_RESPONSE;
			::strncpy(msgp->pubsub.topic, adv_msg.topic, NamingTraits<Topic>::MAX_LENGTH);
			memcpy(msgp->pubsub.raw_params, &(adv_msg.rtcan_id), sizeof(adv_msg.rtcan_id));
			msgp->pubsub.transportp = this;
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
	if (rtcan_msg_p == NULL)
		return false;

	rtcan_msg_p->id = 123 << 8;
	// FIXME need callback to free the message header
	rtcan_msg_p->callback = reinterpret_cast<rtcan_msgcallback_t>(free_header);
	rtcan_msg_p->params = this;
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
	adv_tx_msg.rtcan_id = 0;

	return send_adv_msg(adv_tx_msg);
}

bool RTCANTransport::send_subscription_request(const Topic &topic) {

	adv_tx_msg.type = 'S';
	adv_tx_msg.queue_length = topic.get_max_queue_length();
	::strncpy(adv_tx_msg.topic, topic.get_name(), NamingTraits<Topic>::MAX_LENGTH);
	adv_tx_msg.rtcan_id = 0;

	return send_adv_msg(adv_tx_msg);
}

bool RTCANTransport::send_subscription_response(const Topic &topic) {

	adv_tx_msg.type = 'E';
	adv_tx_msg.queue_length = 0;
	::strncpy(adv_tx_msg.topic, topic.get_name(), NamingTraits<Topic>::MAX_LENGTH);
	// TODO: ID arbitration
	adv_tx_msg.rtcan_id = topic_id(topic.get_name());

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
	if (rtcan_msg_p == NULL)
		return false;

	rtcan_msg_p->id = rsubp->rtcan_id;
	rtcan_msg_p->callback = reinterpret_cast<rtcan_msgcallback_t>(send_cb);
	rtcan_msg_p->params = rsubp;
	rtcan_msg_p->size = rsubp->get_topic()->get_size();
	rtcan_msg_p->status = RTCAN_MSG_READY;
	rtcan_msg_p->data = msgp->get_raw_data();

	rtcanTransmitI(&rtcan, rtcan_msg_p, 100);

	return true;
}

void RTCANTransport::send_cb(rtcan_msg_t &rtcan_msg) {
//  RTCANTransport &transport = reinterpret_cast<RTCANTransport &>(rtcan_msg.params);
//  transport.recv_adv_msg(reinterpret_cast<const adv_msg_t &>(rtcan_msg.data));

	RTCANSubscriber * rsubp = (RTCANSubscriber *) rtcan_msg.params;
	RTCANTransport * transport = (RTCANTransport *) rsubp->get_transport();

	Message * msg = (Message *) (rtcan_msg.data - 1);

	rsubp->release_unsafe(*msg);
	transport->header_pool.free_unsafe(&rtcan_msg);
}

void RTCANTransport::recv_cb(rtcan_msg_t &rtcan_msg) {
	RTCANPublisher* rpub = (RTCANPublisher *) rtcan_msg.params;
	Message * msgp;

	msgp = (Message *) (((uint8_t *) rtcan_msg.data) - sizeof(Message));
	rpub->publish_unsafe(*msgp);
}

void RTCANTransport::initialize(const RTCANConfig &rtcan_config) {

	rtcanInit();
	rtcanStart(&rtcan, &rtcan_config);

	rtcanReceive(&rtcan, &adv_rx_header);

	advertise(mgmt_rpub, "R2P", Time::INFINITE, sizeof(MgmtMsg));
	subscribe(mgmt_rsub, "R2P", mgmt_msgbuf, MGMT_BUFFER_LENGTH, sizeof(MgmtMsg));

	Middleware::instance.add(*this);
}

RemotePublisher *RTCANTransport::create_publisher(Topic &topic, uint8_t * raw_params) const {
	RTCANPublisher * rpubp = new RTCANPublisher();

	rpubp->rtcan_header.id = *(rtcan_id_t *)raw_params;
	rpubp->rtcan_header.callback = reinterpret_cast<rtcan_msgcallback_t>(recv_cb);
	rpubp->rtcan_header.params = rpubp;
	rpubp->rtcan_header.size = 4; // FIXME
	rpubp->rtcan_header.status = RTCAN_MSG_READY;

	rtcanReceive(&RTCAND1, &rpubp->rtcan_header);
	return rpubp;
}

RemoteSubscriber *RTCANTransport::create_subscriber(Topic &topic, Transport &transport,
		TimestampedMsgPtrQueue::Entry queue_buf[], size_t queue_length, uint8_t * raw_params) const {
	RTCANSubscriber * rsubp = new RTCANSubscriber(static_cast<RTCANTransport &>(transport), queue_buf, queue_length);

	// TODO: dynamic ID arbitration
	// FIXME: ID da raw_params se esiste gia' (altri publisher su stesso topic)
	rsubp->rtcan_id = topic_id(topic.get_name());

	return rsubp;
}

// FIXME: to implement
rtcan_id_t RTCANTransport::topic_id(const char * namep) const {
	if (strcmp(namep, "led2") == 0)
		return 112 << 8;
	if (strcmp(namep, "led3") == 0)
		return 113 << 8;
	if (strcmp(namep, "ledall") == 0)
		return 114 << 8;

	if (strcmp(namep, "tilt") == 0)
		return 155 << 8;

	return 255 << 8;
}


void RTCANTransport::free_header(rtcan_msg_t &rtcan_msg) {
	RTCANTransport * transport = (RTCANTransport *) rtcan_msg.params;
	transport->header_pool.free_unsafe(&rtcan_msg);
}

RTCANTransport::RTCANTransport(RTCANDriver &rtcan) :
		Transport(), rtcan(rtcan), header_pool(header_buffer, 10), mgmt_rsub(*this, mgmt_msgqueue_buf,
				MGMT_BUFFER_LENGTH), mgmt_rpub() {
	adv_rx_header.id = 123 << 8;
	adv_rx_header.data = reinterpret_cast<uint8_t *>(&adv_rx_msg);
	adv_rx_header.size = sizeof(adv_rx_msg);
	adv_rx_header.callback = reinterpret_cast<rtcan_msgcallback_t>(adv_rx_cb);
	adv_rx_header.params = reinterpret_cast<void *>(this);
	adv_rx_header.status = RTCAN_MSG_READY;
}

RTCANTransport::~RTCANTransport() {
}

} // namespace r2p
