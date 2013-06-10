#ifndef __REMOTESUBSCRIBERT_HPP__
#define __REMOTESUBSCRIBERT_HPP__

#include "ch.hpp"

#include "RemoteSubscriber.hpp"

template <typename T, int N>
class RemoteSubscriberT : public RemoteSubscriber {
private:
	uint8_t _buffer[N*MEM_ALIGN_NEXT(sizeof(T))] __attribute__((aligned(sizeof(stkalign_t))));
	T * _queue_buffer[N];
public:
	RemoteSubscriberT(const char *);
	int size(void);
	T * get(void);
};

template <typename T, int N>
RemoteSubscriberT<T,N>::RemoteSubscriberT(const char * topic) : RemoteSubscriber(topic, sizeof(T), _buffer, (BaseMessage **)_queue_buffer, N) {

}

template <typename T, int N>
int RemoteSubscriberT<T,N>::size(void) {
	return N;
}

template <typename T, int N>
T * RemoteSubscriberT<T,N>::get(void) {
	return static_cast<T *>(BaseSubscriber::get());
}

#endif
