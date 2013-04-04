#ifndef __BASESUBSCRIBER_HPP__
#define __BASESUBSCRIBER_HPP__

#include "ch.h"
#include "BaseMessage.hpp"
#include "MessageQueue.hpp"
#include "BasePublisher.hpp"

typedef void (*callback_t)(BaseMessage *);

class BaseSubscriber {
protected:
	const char * _topic;
	uint32_t _msg_size;
	BasePublisher * _source;
	MessageQueue _queue;
	// FIXME to substitute with memory pool growth (from heap) on publisher side?
	uint8_t * _buffer;
public:
	BaseSubscriber(const char * topic, size_t msg_size, uint8_t * buffer,
			BaseMessage ** queue_buffer, uint32_t queue_size);
	const char * topic(void);
	BasePublisher * source(void);
	BaseSubscriber *last(void);
	BaseMessage *get(void);
	void release(BaseMessage *);
	void releaseI(BaseMessage *);
	uint16_t size(void);
};

#endif
