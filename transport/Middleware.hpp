#ifndef __MIDDLEWARE_HPP__
#define __MIDDLEWARE_HPP__

#include "ch.hpp"

#include "BaseMessage.hpp"
#include "MessageQueue.hpp"
#include "BasePublisher.hpp"
#include "LocalPublisher.hpp"
#include "RemotePublisher.hpp"
#include "Publisher.hpp"
#include "BaseSubscriber.hpp"
#include "LocalSubscriber.hpp"
#include "Subscriber.hpp"
#include "RemoteSubscriber.hpp"
#include "RemoteSubscriberT.hpp"
#include "Node.hpp"

#include "r2config.h"

class Middleware {
private:
	static Middleware _instance;
	static Config::Transport & _transport;
	Node * _nodes;
	RemotePublisher * _remote_publishers;

	Middleware(void);
	Middleware(Middleware const&);
	void operator=(Middleware const&);

public:
	static Middleware & instance(void);
	static Config::Transport & transport(void);
	void newNode(Node *);
	void delNode(Node *);
	bool advertise(LocalPublisher *);
	bool advertise(RemotePublisher *);
	bool_t compareTopics(const char *, const char *);
	LocalPublisher * findLocalPublisher(const char *);
	RemotePublisher * findRemotePublisher(const char *);
};

/*
 * Middleware declaration
 */

#endif /* MIDDLEWARE_HPP_ */
