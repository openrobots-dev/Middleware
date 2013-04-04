#ifndef __LOCALPUBLISHER_HPP__
#define __LOCALPUBLISHER_HPP__

#include "ch.h"

class BaseMessage;
class LocalSubscriber;
class RemoteSubscriber;

class LocalPublisher : public BasePublisher {
protected:
	LocalPublisher * _next;
	LocalSubscriber * _subscribers;
	RemoteSubscriber * _remote_subscribers;
public:
	LocalPublisher(const char *, size_t);
	LocalPublisher * next(void);
	void next(LocalPublisher *);
	int broadcast(BaseMessage *);
	void subscribe(LocalSubscriber *, void *, size_t);
	void subscribe(RemoteSubscriber *, void *, size_t);
};

#endif /* __LOCALPUBLISHER_HPP__ */
