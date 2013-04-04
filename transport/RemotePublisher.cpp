#include "ch.hpp"

#include "BasePublisher.hpp"
#include "RemotePublisher.hpp"
#include "LocalSubscriber.hpp"

#include "transport/RTcan.hpp"
#include "rtcan.h"

extern RTCANDriver RTCAND;

RemotePublisher::RemotePublisher(const char * topic, size_t msg_size) : BasePublisher(topic, msg_size) {
	_next = NULL;
	_subscribers = NULL;

	_rtcan_msg.id = 999;
	_rtcan_msg.type = RTCAN_SRT;
	_rtcan_msg.callback = RTcan::rxcb;
	_rtcan_msg.params = this;
	_rtcan_msg.size = msg_size;
	_rtcan_msg.status = RTCAN_MSG_READY;
}

RemotePublisher * RemotePublisher::next(void) {
	return _next;
}

void RemotePublisher::next(RemotePublisher * next) {
	_next = next;
}

int RemotePublisher::broadcastI(BaseMessage * msg) {
	LocalSubscriber * sub = _subscribers;
	int n = 0;

	while (sub != NULL) {
		sub = sub->notify(msg, n);
	}

	// lo fa da solo quando esce da ISR
	//chSchRescheduleS();

	return n;
}

void RemotePublisher::subscribe(LocalSubscriber * sub, void * buffer,
		size_t size) {

	chPoolLoadArray(&_pool, buffer, size);

	chSysLock();
	if (this->_subscribers == NULL) {
		this->_subscribers = sub;
	} else {
		sub->link(_subscribers);
		_subscribers = sub;
	}

	_rtcan_msg.data = (uint8_t *) allocI();
	if (_rtcan_msg.data) {
		rtcanRegister(&RTCAND, &_rtcan_msg);
	}

	chSysUnlock();
}

void RemotePublisher::id(uint16_t id) {
	_rtcan_msg.id = id;
}

uint16_t RemotePublisher::id(void) {
	return _rtcan_msg.id;
}


