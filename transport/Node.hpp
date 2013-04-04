#ifndef __NODE_HPP__
#define __NODE_HPP__

#include "BasePublisher.hpp"
#include "BaseSubscriber.hpp"

class Node {
protected:
	const char * _name;
	Node * _next;
	LocalPublisher * _publishers;
	LocalSubscriber * _subscribers;
	LocalSubscriber * _queued_subscribers;
public:
	Node (const char * topic);
	Node * next(void);
	void next(Node * next);
	bool advertise(LocalPublisher * pub);
	bool subscribe(LocalSubscriber * sub);
	bool localSubscribe(LocalPublisher * pub, LocalSubscriber * sub);
	bool remoteSubscribe(RemotePublisher * pub, LocalSubscriber * sub);
	LocalPublisher * publishers(void);
	LocalSubscriber * queue(void);
	void queueSubscriber(LocalSubscriber * sub);
	void unqueueSubscriber(LocalSubscriber * sub);
	void spin(systime_t timeout);
	void spin(void);
};

#endif
