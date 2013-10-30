#include <r2p/transport/RTCANTransport.hpp>
#include <r2p/transport/RTCANPublisher.hpp>
#include <r2p/transport/RTCANSubscriber.hpp>
#include <r2p/Middleware.hpp>

#include <rtcan.h>

//FIXME needed for header pool, should use abstract allocator
#include <ch.h>
#include <hal.h>

namespace r2p {

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
	RTCANSubscriber * rsubp = reinterpret_cast<RTCANSubscriber *>(rtcan_msg.params);
	RTCANTransport * transport = reinterpret_cast<RTCANTransport *>(rsubp->get_transport());
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
		size_t queue_length, uint8_t *raw_params) const {
	RTCANSubscriber *rsubp = new RTCANSubscriber(*const_cast<RTCANTransport *>(this), queue_buf, queue_length);

	// TODO: dynamic ID arbitration

	rsubp->rtcan_id = topic_id(topic.get_name());

	return rsubp;
}

void RTCANTransport::fill_raw_params(const Topic &topic, uint8_t *raw_paramsp) {
	*reinterpret_cast<rtcan_id_t *>(raw_paramsp) = topic_id(topic.get_name());
}

void RTCANTransport::initialize(const RTCANConfig &rtcan_config) {
	Topic & mgmt_topic = Middleware::instance.get_mgmt_topic();
	rtcan_id_t rtcan_id = topic_id(mgmt_topic.get_name());

	rtcanInit();
	rtcanStart(&rtcan, &rtcan_config);

	mgmt_rsub = reinterpret_cast<RTCANSubscriber *>(create_subscriber(mgmt_topic, mgmt_msgqueue_buf, MGMT_BUFFER_LENGTH, (uint8_t *)&rtcan_id));
	subscribe(*mgmt_rsub, "R2P", mgmt_msgbuf, MGMT_BUFFER_LENGTH, sizeof(MgmtMsg));

	mgmt_rpub = reinterpret_cast<RTCANPublisher *>(create_publisher(mgmt_topic, (const uint8_t *)&rtcan_id));
	advertise(*mgmt_rpub, "R2P", Time::INFINITE, sizeof(MgmtMsg));

	Middleware::instance.add(*this);
}

RTCANTransport::RTCANTransport(RTCANDriver &rtcan) :
		Transport("rtcan"), rtcan(rtcan), header_pool(header_buffer, 10) {
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
