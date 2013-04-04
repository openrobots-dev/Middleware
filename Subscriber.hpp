#ifndef __SUBSCRIBER_HPP__
#define __SUBSCRIBER_HPP__

#include "ch.hpp"

#include "LocalSubscriber.hpp"

template <typename T, int N>
class Subscriber : public LocalSubscriber {
private:
	uint8_t _buffer[N*MEM_ALIGN_NEXT(sizeof(T))] __attribute__((aligned(sizeof(stkalign_t))));
	T * _queue_buffer[N];
public:
	Subscriber(const char *, callback_t = NULL);
	int size(void);
	T * get(void);
};



template <class T, int N>
Subscriber<T,N>::Subscriber(const char * topic, callback_t callback) : LocalSubscriber(topic, sizeof(T), _buffer, (BaseMessage **)_queue_buffer, N, callback) {

}

template <class T, int N>
int Subscriber<T,N>::size(void) {
	return N;
}

template <class T, int N>
T * Subscriber<T,N>::get(void) {
	return static_cast<T *>(BaseSubscriber::get());
}

#endif
