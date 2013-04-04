#ifndef __LEDTRANSPORT_HPP__
#define __LEDTRANSPORT_HPP__

#include "BaseMessage.hpp"
#include "RemoteSubscriber.hpp"

class LedTransport
{
private:
	static LedTransport _instance;
	int _cnt;

	/* Singleton */
	LedTransport(void);
	LedTransport(LedTransport const&);
	void operator=(LedTransport const&);

public:
	static LedTransport & instance(void);
    bool send(RemoteSubscriber* sub, BaseMessage* msg, int size);
};

#endif /* __LEDTRANSPORT_HPP__ */
