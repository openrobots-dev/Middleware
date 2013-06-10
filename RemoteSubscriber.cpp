#include "ch.hpp"

#include "Middleware.hpp"

RemoteSubscriber::RemoteSubscriber(const char * topic, size_t msg_size,
		uint8_t * buffer, BaseMessage ** queue_buffer, uint32_t queue_size) : BaseSubscriber(topic, msg_size, buffer, queue_buffer, queue_size) {
	_next = NULL;
	_next_notify = NULL;
	_id = 999;
}

RemoteSubscriber * RemoteSubscriber::next(void) {
	return _next;
}

void RemoteSubscriber::next(RemoteSubscriber * next) {
	_next = next;
}

void RemoteSubscriber::link(RemoteSubscriber * sub) {
	_next_notify = sub;
}

/* called from within lock */
RemoteSubscriber * RemoteSubscriber::notify(BaseMessage *msg) {
	int n;
	return this->notify(msg, n);
}

RemoteSubscriber * RemoteSubscriber::notify(BaseMessage *msg, int &n) {
	Middleware & mw = Middleware::instance();

	/* Put message on queue. */
	if(_queue.putI(msg)) {
		// TODO: put & get, qui basterebbe cnt, oppure si segnala il transport e fa lui la get (ma da quale sorgente?)
		mw.transport().send(this, _queue.getI(), _msg_size);
		msg->reference();
		n++;
	}

	return _next_notify;
}

void RemoteSubscriber::subscribe(LocalPublisher * publisher) {
	_source = publisher;
	publisher->subscribe(this, _buffer, _queue.sizeI());
}

void RemoteSubscriber::id(uint16_t id) {
	_id = id;
}

uint16_t RemoteSubscriber::id(void) {
	return _id;
}


