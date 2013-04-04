#ifndef __REMOTESUBSCRIBER_HPP__
#define __REMOTESUBSCRIBER_HPP__

#include "ch.h"
#include "BaseMessage.hpp"
#include "BasePublisher.hpp"
#include "LocalPublisher.hpp"
#include "BaseSubscriber.hpp"

class RemoteSubscriber : public BaseSubscriber {
private:
	// TODO: this should depend on the Transport, typedef needed.
	uint16_t _id;
protected:
	RemoteSubscriber * _next;
	RemoteSubscriber * _next_notify;
public:
	RemoteSubscriber(const char * topic, size_t msg_size, uint8_t * buffer, BaseMessage ** queue_buffer,  uint32_t queue_size);
	RemoteSubscriber * next(void);
	void next(RemoteSubscriber *);
	void link(RemoteSubscriber *);
	RemoteSubscriber *notify(BaseMessage *);
	RemoteSubscriber *notify(BaseMessage *, int &);
	void subscribe(LocalPublisher *);
	void id(uint16_t id);
	uint16_t id(void);
};

#endif /* __REMOTESUBSCRIBER_HPP__ */
