#ifndef __REMOTEPUBLISHER_HPP__
#define __REMOTEPUBLISHER_HPP__

#include "ch.h"
#include "rtcan.h"
#include "BaseMessage.hpp"
#include "BasePublisher.hpp"
#include "BaseSubscriber.hpp"

class RemotePublisher : public BasePublisher {
private:
	RemotePublisher * _next;
	LocalSubscriber * _subscribers;
	rtcan_msg_t _rtcan_msg;
public:
	RemotePublisher(const char * t, size_t msg_size);
	RemotePublisher * next(void);
	void next(RemotePublisher *);
	int broadcastI(BaseMessage *);
	void subscribe(LocalSubscriber *, void *, size_t);
	void id(uint16_t id);
	uint16_t id(void);
};

#endif /* __REMOTEPUBLISHER_HPP__ */
