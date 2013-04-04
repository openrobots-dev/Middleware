#ifndef __PUBLISHER_HPP__
#define __PUBLISHER_HPP__

#include "BasePublisher.hpp"
#include "LocalPublisher.hpp"

template <typename T> class Publisher : public LocalPublisher {
private:
public:
	Publisher(const char *);
	T * alloc(void);
	T * allocI(void);
};

template <typename T> Publisher<T>::Publisher(const char * t) : LocalPublisher(t, sizeof(T)) {

}

template <typename T> T * Publisher<T>::alloc(void) {
	return static_cast<T *>(LocalPublisher::alloc());
}

#endif
