
#include <r2p/transport/RTCANTransport.hpp>
#include <r2p/transport/RTCANPublisher.hpp>
#include <r2p/transport/RTCANSubscriber.hpp>
#include <r2p/Middleware.hpp>

#include <rtcan.h>

//FIXME needed for header pool, should use abstract allocator
#include <ch.h>
#include <hal.h>

namespace r2p {

RTCANTransport * myrtcan; // FIXME !!!!

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
#if R2P_USE_BRIDGE_MODE
            MessageGuardUnsafe guard(*msgp, *mgmt_rpub.get_topic());
            msgp->set_source(this);
#endif
		    msgp->type = MgmtMsg::CMD_ADVERTISE;
			::strncpy(msgp->pubsub.topic, adv_msg.topic, NamingTraits<Topic>::MAX_LENGTH);
			msgp->pubsub.payload_size = adv_msg.payload_size;

			mgmt_rpub.publish_locally_unsafe(*msgp);
#if R2P_USE_BRIDGE_MODE
			mgmt_rpub.publish_remotely_unsafe(*msgp);
#endif
		}
		break;
	case 'S':
		if (mgmt_rpub.alloc_unsafe(reinterpret_cast<Message *&>(msgp))) {
#if R2P_USE_BRIDGE_MODE
            MessageGuardUnsafe guard(*msgp, *mgmt_rpub.get_topic());
            msgp->set_source(this);
#endif
			msgp->type = MgmtMsg::CMD_SUBSCRIBE_REQUEST;
			::strncpy(msgp->pubsub.topic, adv_msg.topic, NamingTraits<Topic>::MAX_LENGTH);
            msgp->pubsub.payload_size = adv_msg.payload_size;
			msgp->pubsub.queue_length = static_cast<uint16_t>(adv_msg.queue_length);

            mgmt_rpub.publish_locally_unsafe(*msgp);
#if R2P_USE_BRIDGE_MODE
            mgmt_rpub.publish_remotely_unsafe(*msgp);
#endif
		}
		break;
	case 'E':
		if (mgmt_rpub.alloc_unsafe(reinterpret_cast<Message *&>(msgp))) {
#if R2P_USE_BRIDGE_MODE
            MessageGuardUnsafe guard(*msgp, *mgmt_rpub.get_topic());
            msgp->set_source(this);
#endif
			msgp->type = MgmtMsg::CMD_SUBSCRIBE_RESPONSE;
			::strncpy(msgp->pubsub.topic, adv_msg.topic, NamingTraits<Topic>::MAX_LENGTH);
			memcpy(msgp->pubsub.raw_params, &(adv_msg.rtcan_id), sizeof(adv_msg.rtcan_id));
            msgp->pubsub.payload_size = adv_msg.payload_size;

            mgmt_rpub.publish_locally_unsafe(*msgp);
#if R2P_USE_BRIDGE_MODE
            mgmt_rpub.publish_remotely_unsafe(*msgp);
#endif
		}
		break;

	default:
		// Unhandled messages
		break;
	}
}

bool RTCANTransport::send_adv_msg(const adv_msg_t &adv_msg) {
	rtcan_msg_t * rtcan_msg_p;

	// Allocate RTCAN msg header
	rtcan_msg_p = header_pool.alloc();
	if (rtcan_msg_p == NULL)
		return false;

	rtcan_msg_p->id = mgmt_rsub.rtcan_id;
	// FIXME need callback to free the message header
	rtcan_msg_p->callback = reinterpret_cast<rtcan_msgcallback_t>(free_header);
	rtcan_msg_p->params = this;
	rtcan_msg_p->size = sizeof(adv_msg_t);
	rtcan_msg_p->status = RTCAN_MSG_READY;
	rtcan_msg_p->data = reinterpret_cast<const uint8_t *>(&adv_msg);

	rtcanTransmit(&rtcan, rtcan_msg_p, 100);

	// FIXME!!!
	while (rtcan_msg_p->status != RTCAN_MSG_READY) {
        Thread::sleep(Time::ms(10));
    }

	return true;
}

bool RTCANTransport::send_advertisement(const Topic &topic) {

	adv_tx_msg.type = 'P';
	adv_tx_msg.queue_length = 0;
	::strncpy(adv_tx_msg.topic, topic.get_name(), NamingTraits<Topic>::MAX_LENGTH);
	adv_tx_msg.payload_size = topic.get_payload_size();
	adv_tx_msg.rtcan_id = 0;

	return send_adv_msg(adv_tx_msg);
}

bool RTCANTransport::send_subscription_request(const Topic &topic) {

	adv_tx_msg.type = 'S';
	adv_tx_msg.queue_length = topic.get_max_queue_length();
	::strncpy(adv_tx_msg.topic, topic.get_name(), NamingTraits<Topic>::MAX_LENGTH);
    adv_tx_msg.payload_size = topic.get_payload_size();
	adv_tx_msg.rtcan_id = 0;

	return send_adv_msg(adv_tx_msg);
}

bool RTCANTransport::send_subscription_response(const Topic &topic) {

	adv_tx_msg.type = 'E';
	adv_tx_msg.queue_length = 0;
	::strncpy(adv_tx_msg.topic, topic.get_name(), NamingTraits<Topic>::MAX_LENGTH);
    adv_tx_msg.payload_size = topic.get_payload_size();
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

bool RTCANTransport::send_bootload() {

	// TODO
	return true;
}

bool RTCANTransport::send(Message * msgp, RTCANSubscriber * rsubp) {
	rtcan_msg_t * rtcan_msg_p;

#if R2P_USE_BRIDGE_MODE
	R2P_ASSERT(msgp->get_source() != this);
#endif

	// Allocate RTCAN msg header
	rtcan_msg_p = header_pool.alloc_unsafe();
	if (rtcan_msg_p == NULL)
		return false;

	rtcan_msg_p->id = rsubp->rtcan_id;
	rtcan_msg_p->callback = reinterpret_cast<rtcan_msgcallback_t>(send_cb);
	rtcan_msg_p->params = rsubp;
	rtcan_msg_p->size = rsubp->get_topic()->get_payload_size();
	rtcan_msg_p->status = RTCAN_MSG_READY;
	rtcan_msg_p->data = msgp->get_raw_data();

	rtcanTransmitI(&rtcan, rtcan_msg_p, 50);
	return true;
}

void RTCANTransport::send_cb(rtcan_msg_t &rtcan_msg) {
//  RTCANTransport &transport = reinterpret_cast<RTCANTransport &>(rtcan_msg.params);
//  transport.recv_adv_msg(reinterpret_cast<const adv_msg_t &>(rtcan_msg.data));

	RTCANSubscriber * rsubp = (RTCANSubscriber *) rtcan_msg.params;
	RTCANTransport * transport = (RTCANTransport *) rsubp->get_transport();
	Message &msg = const_cast<Message &>(Message::get_msg_from_raw_data(rtcan_msg.data));

	rsubp->release_unsafe(msg);
	transport->header_pool.free_unsafe(&rtcan_msg);
}

void RTCANTransport::recv_cb(rtcan_msg_t &rtcan_msg) {
	RTCANPublisher* pubp = static_cast<RTCANPublisher *>(rtcan_msg.params);
	Message *msgp = const_cast<Message *>(&Message::get_msg_from_raw_data(rtcan_msg.data));

#if !R2P_USE_BRIDGE_MODE
    pubp->publish_locally_unsafe(*msgp);
#else
	R2P_ASSERT(pubp->get_transport() != NULL);
	MessageGuardUnsafe(*msgp, *pubp->get_topic());
	msgp->set_source(pubp->get_transport());
    pubp->publish_locally_unsafe(*msgp);
	pubp->publish_remotely_unsafe(*msgp);
#endif

	// allocate again for next message from RTCAN
	if (pubp->alloc_unsafe(msgp)) {
		rtcan_msg.data = msgp->get_raw_data();
	} else {
		rtcan_msg.status = RTCAN_MSG_BUSY;
//		R2P_ASSERT(false);
	}
}

void RTCANTransport::free_header(rtcan_msg_t &rtcan_msg) {
	RTCANTransport * transport = (RTCANTransport *) rtcan_msg.params;
	transport->header_pool.free_unsafe(&rtcan_msg);
}


RemotePublisher *RTCANTransport::create_publisher(Topic &topic, const uint8_t *raw_params) const {
	RTCANPublisher * rpubp = new RTCANPublisher(*const_cast<RTCANTransport *>(this));
	Message * msgp;
	bool success;

	R2P_ASSERT(rpubp != NULL);

	success = topic.alloc(msgp);
	R2P_ASSERT(success);

	rtcan_msg_t * rtcan_headerp = &(rpubp->rtcan_header);
	rtcan_headerp->id = *(rtcan_id_t *) raw_params;
	rtcan_headerp->callback = reinterpret_cast<rtcan_msgcallback_t>(recv_cb);
	rtcan_headerp->params = rpubp;
	rtcan_headerp->size = topic.get_payload_size();
	rtcan_headerp->data = msgp->get_raw_data();
	rtcan_headerp->status = RTCAN_MSG_READY;

	rtcanReceiveMask(&RTCAND1, rtcan_headerp, 0xFF00);
	return rpubp;
}

RemoteSubscriber *RTCANTransport::create_subscriber(Topic &topic, TimestampedMsgPtrQueue::Entry queue_buf[],
		size_t queue_length, const uint8_t *raw_params) const {
	RTCANSubscriber *rsubp = new RTCANSubscriber(*const_cast<RTCANTransport *>(this), queue_buf, queue_length);

	// TODO: dynamic ID arbitration
	// FIXME: ID da raw_params se esiste gia' (altri publisher su stesso topic)
	(void)raw_params;
	rsubp->rtcan_id = topic_id(topic.get_name());

	return rsubp;
}


void RTCANTransport::initialize(const RTCANConfig &rtcan_config) {

	rtcanInit();
	rtcanStart(&rtcan, &rtcan_config);


	advertise(mgmt_rpub, "R2P", Time::INFINITE, sizeof(MgmtMsg));
	subscribe(mgmt_rsub, "R2P", mgmt_msgbuf, MGMT_BUFFER_LENGTH, sizeof(MgmtMsg));

	mgmt_rsub.rtcan_id = topic_id(mgmt_rsub.get_topic()->get_name());

	adv_rx_header.id = topic_id(mgmt_rsub.get_topic()->get_name());
	adv_rx_header.data = reinterpret_cast<uint8_t *>(&adv_rx_msg);
	adv_rx_header.size = sizeof(adv_rx_msg);
	adv_rx_header.callback = reinterpret_cast<rtcan_msgcallback_t>(adv_rx_cb);
	adv_rx_header.params = reinterpret_cast<void *>(this);
	adv_rx_header.status = RTCAN_MSG_READY;

	rtcanReceiveMask(&rtcan, &adv_rx_header, 0xFF00);

	Middleware::instance.add(*this);
}

RTCANTransport::RTCANTransport(RTCANDriver &rtcan) :
		Transport("rtcan"), rtcan(rtcan), header_pool(header_buffer, 10), mgmt_rsub(*this, mgmt_msgqueue_buf,
				MGMT_BUFFER_LENGTH), mgmt_rpub(*this) {
}

RTCANTransport::~RTCANTransport() {
}



// FIXME: to implement

#include <r2p/msg/led.hpp>
#include <r2p/msg/motor.hpp>
#include <r2p/msg/imu.hpp>

rtcan_id_t RTCANTransport::topic_id(const char * namep) const {
	if (strncmp(namep, "R2P", NamingTraits<Topic>::MAX_LENGTH) == 0)
		return 1 << 8 | stm32_id8();
	if (strncmp(namep, "leds", NamingTraits<Topic>::MAX_LENGTH) == 0)
		return LEDS_ID | stm32_id8();

	if (strncmp(namep, "pwm2", NamingTraits<Topic>::MAX_LENGTH) == 0)
		return PWM2_ID | stm32_id8();
	if (strncmp(namep, "qei", NamingTraits<Topic>::MAX_LENGTH) == 0)
		return QEI_ID | stm32_id8();

	if (strncmp(namep, "tilt", NamingTraits<Topic>::MAX_LENGTH) == 0)
		return TILT_ID | stm32_id8();

	if (strncmp(namep, "velocity", NamingTraits<Topic>::MAX_LENGTH) == 0)
		return VELOCITY_ID | stm32_id8();

	return 255 << 8;
}


} // namespace r2p
