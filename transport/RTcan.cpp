#include "ch.h"
#include "hal.h"

#include "RTcan.hpp"

#include "RemotePublisher.hpp"

RTcan RTcan::_instance;

RTcan::RTcan(void) {
	chPoolInit(&_header_pool, sizeof(rtcan_msg_t), NULL);
	chPoolLoadArray(&_header_pool, _header_buffer, N);
}

RTcan & RTcan::instance(void) {
	return _instance;
}

void RTcan::free(rtcan_msg_t* rtcan_msg_p) {
	chPoolFreeI(&_header_pool, rtcan_msg_p);
}

bool RTcan::send(RemoteSubscriber* sub, BaseMessage* msg, int size) {
	rtcan_msg_t * rtcan_msg_p;

	rtcan_msg_p = (rtcan_msg_t *) chPoolAllocI(&_header_pool);

	if (rtcan_msg_p == NULL) {
		return false;
	}

	rtcan_msg_p->id = sub->id();
	rtcan_msg_p->type = RTCAN_SRT;
//FIXME XXX
	rtcan_msg_p->callback = sendcb;
	rtcan_msg_p->params = sub;
	// FIXME
	rtcan_msg_p->size = size - sizeof(BaseMessage);
	rtcan_msg_p->status = RTCAN_MSG_READY;
	rtcan_msg_p->data = ((uint8_t *) msg) + sizeof(BaseMessage);

	rtcanSendSrtI(rtcan_msg_p, 100);

	return true;
}

void RTcan::sendcb(rtcan_msg_t* rtcan_msg_p) {
	RemoteSubscriber* sub = static_cast<RemoteSubscriber*>(rtcan_msg_p->params);

	BaseMessage * msg_p;

	msg_p = (BaseMessage *)(((uint8_t *) rtcan_msg_p->data) - sizeof(BaseMessage));
	sub->releaseI(msg_p);

	RTcan::instance().free(rtcan_msg_p);
}

void RTcan::rxcb(rtcan_msg_t* rtcan_msg_p) {
	RemotePublisher* pub = static_cast<RemotePublisher*>(rtcan_msg_p->params);

	BaseMessage * msg;

	msg = (BaseMessage *)(((uint8_t *) rtcan_msg_p->data) - sizeof(BaseMessage));
	pub->broadcastI(msg);
}

