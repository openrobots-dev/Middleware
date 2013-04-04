#ifndef __LOCALSUBSCRIBER_HPP__
#define __LOCALSUBSCRIBER_HPP__

#include "ch.h"
#include "BaseMessage.hpp"
#include "BasePublisher.hpp"
#include "BaseSubscriber.hpp"
#include "LocalPublisher.hpp"
#include "RemotePublisher.hpp"

typedef void (*callback_t)(BaseMessage *);

class LocalSubscriber: public BaseSubscriber {
protected:
	LocalSubscriber * _next;
	LocalSubscriber * _next_notify;
	Thread * _tp;
	eventmask_t _mask;
	callback_t _callback;
public:
	LocalSubscriber(const char * topic, size_t msg_size, uint8_t * buffer,
			BaseMessage ** queue_buffer, uint32_t queue_size, callback_t callback);
	LocalSubscriber * next(void);
	void next(LocalSubscriber* sub);
	void link(LocalSubscriber* sub);
	LocalSubscriber *notify(BaseMessage* msg);
	LocalSubscriber *notify(BaseMessage* msg, int& n);
	void subscribe(LocalPublisher* pub, eventmask_t emask);
	void subscribe(RemotePublisher* pub, eventmask_t emask);
	callback_t callback(void);
	void callback(BaseMessage* msg);
};

#endif /* __LOCALSUBSCRIBER_HPP__ */
