#ifndef __RTCAN_HPP__
#define __RTCAN_HPP__

#include "ch.h"
#include "hal.h"

#include "BaseMessage.hpp"
#include "RemoteSubscriber.hpp"

#include "rtcan.h"

// FIXME !!!!!!
#define N 10

class RTcan
{
private:
	static RTcan _instance;
	uint8_t _header_buffer[N*MEM_ALIGN_NEXT(sizeof(rtcan_msg_t))] __attribute__((aligned(sizeof(stkalign_t))));
	MemoryPool _header_pool;

	/* Singleton */
	RTcan(void);
	RTcan(RTcan const&);
	void operator=(RTcan const&);

	void free(rtcan_msg_t* rtcan_msg_p);
public:
	static RTcan & instance(void);
    bool send(RemoteSubscriber* sub, BaseMessage* msg, int size);
	static void sendcb(rtcan_msg_t* rtcan_msg);
	static void rxcb(rtcan_msg_t* rtcan_msg);
};

#endif /* __RTCAN_HPP__ */
