#include "ch.hpp"

#include "BaseMessage.hpp"
#include "MessageQueue.hpp"
#include "BaseSubscriber.hpp"

BaseSubscriber::BaseSubscriber(const char * topic, size_t msg_size, uint8_t * buffer, BaseMessage ** queue_buffer,  uint32_t queue_size)
	: _queue(queue_buffer, queue_size)
	{
	_topic = topic;
	_msg_size = msg_size;
	_source = NULL;
	_buffer = buffer;
}

const char * BaseSubscriber::topic(void) {
	return _topic;
}

BasePublisher * BaseSubscriber::source(void) {
	return _source;
}

BaseMessage * BaseSubscriber::get(void) {
	return _queue.get();
}

void BaseSubscriber::release(BaseMessage *d) {
	_source->release(d);
}

void BaseSubscriber::releaseI(BaseMessage *d) {
	_source->releaseI(d);
}

uint16_t BaseSubscriber::size(void) {
	return _queue.size();
}
