#include "BaseMessage.hpp"
#include "MessageQueue.hpp"

MessageQueue::MessageQueue(BaseMessage ** buffer, uint32_t size) {
	_buffer = _write = _read = buffer;
	_top = &buffer[size];
	_cnt = 0;
}

uint32_t MessageQueue::size(void) {
	uint32_t size;

	chSysLock();
	size = sizeI();
	chSysUnlock();

	return size;
}

uint32_t MessageQueue::sizeI(void) {
	return (_top - _buffer);
}

bool_t MessageQueue::put(BaseMessage * msg) {
	bool_t ret;

	chSysLock();
	ret = putI(msg);
	chSysUnlock();

	return ret;
}

bool_t MessageQueue::putI(BaseMessage * msg) {

	if (_cnt > sizeI()) {
		return false;
	}

	*_write++ = msg;
	_cnt++;

	if (_write >= _top) {
		_write = _buffer;
	}

	return true;
}

BaseMessage * MessageQueue::get(void) {
	BaseMessage * msg;

	chSysLock();
	msg = getI();
	chSysUnlock();

	return msg;
}

BaseMessage * MessageQueue::getI(void) {
	BaseMessage * msg;

	if (_cnt == 0) {
		return 0;
	}

	msg = *_read++;
	_cnt--;

	if (_read >= _top) {
		_read = _buffer;
	}

	return msg;
}
