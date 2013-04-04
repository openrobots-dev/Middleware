#ifndef __BASEPUBLISHER_HPP__
#define __BASEPUBLISHER_HPP__

#include "ch.h"

class BaseMessage;
class LocalSubscriber;

class BasePublisher {
protected:
	const char * _topic;
	MemoryPool _pool;
public:
	BasePublisher(const char * topic, size_t msg_size);
	const char * topic(void);
	BaseMessage * alloc(void);
	BaseMessage * allocI(void);
	void release(BaseMessage *);
	void releaseI(BaseMessage *);
};

#endif
