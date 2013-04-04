#include "ch.hpp"

#include "Middleware.hpp"

#include "hal.h"
#include "board.h"

#include "BasePublisher.hpp"

BasePublisher::BasePublisher(const char * t, size_t msg_size) {
	_topic = t;
	chPoolInit(&_pool, msg_size, NULL);
}

const char * BasePublisher::topic(void) {
	return _topic;
}

BaseMessage * BasePublisher::alloc(void) {
	BaseMessage * msg;

#ifdef R2MW_TEST
	palSetPad(TEST_GPIO, TEST1);
#endif /* R2MW_TEST */

	chSysLock();
	msg = allocI();
	chSysUnlock();

	return msg;
}

BaseMessage * BasePublisher::allocI(void) {
	BaseMessage * msg;

	msg = (BaseMessage *) chPoolAllocI(&_pool);

	/* reset reference count */
	if (msg != NULL)
		msg->reset();

	return msg;
}

void BasePublisher::release(BaseMessage* msg) {
	chSysLock();
	releaseI(msg);
	chSysUnlock();
}

extern Thread * tp;
void BasePublisher::releaseI(BaseMessage* msg) {
	/* if the reference count is reduced to zero then free the memory */
	if (msg->dereference()) {
		chPoolAddI(&_pool, msg);
#ifdef R2MW_TEST
		palClearPad(TEST_GPIO, TEST1);
//		if (tp != NULL) {
//			chSchReadyI(tp);
//			tp = NULL;
//		}
#endif /* R2MW_TEST */
	}
}
