#include "ch.hpp"

#include "Middleware.hpp"

#include "hal.h"
#include "board.h"

#include "BasePublisher.hpp"

LocalPublisher::LocalPublisher(const char * topic, size_t msg_size) : BasePublisher(topic, msg_size) {
	_next = NULL;
	_subscribers = NULL;
	_remote_subscribers = NULL;
}

LocalPublisher * LocalPublisher::next(void) {
	return _next;
}

void LocalPublisher::next(LocalPublisher * next) {
	_next = next;
}

int LocalPublisher::broadcast(BaseMessage * msg) {
	LocalSubscriber * next = _subscribers;
	RemoteSubscriber * rnext = _remote_subscribers;
	int n = 0;

	chSysLock();

	while (next != NULL) {
		next = next->notify(msg, n);
	}

	while (rnext != NULL) {
		rnext = rnext->notify(msg, n);
	}

	chSchRescheduleS();
	chSysUnlock();

	return n;
}

void LocalPublisher::subscribe(LocalSubscriber * sub, void * buffer,
		size_t size) {

	chPoolLoadArray(&_pool, buffer, size);
	chSysLock();
	if (this->_subscribers == NULL) {
		this->_subscribers = sub;
	} else {
		sub->link(_subscribers);
		_subscribers = sub;
	}
	chSysUnlock();
}

void LocalPublisher::subscribe(RemoteSubscriber * sub, void * buffer,
		size_t size) {

	chPoolLoadArray(&_pool, buffer, size);
	chSysLock();
	if (this->_remote_subscribers == NULL) {
		this->_remote_subscribers = sub;
	} else {
		sub->link(_remote_subscribers);
		_remote_subscribers = sub;
	}
	chSysUnlock();
}
