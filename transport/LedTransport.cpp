#include "ch.h"
#include "hal.h"

#include "LedTransport.hpp"


LedTransport LedTransport::_instance;

LedTransport::LedTransport(void) {
	_cnt = 0;
}

LedTransport & LedTransport::instance(void) {
	return _instance;
}

bool LedTransport::send(RemoteSubscriber* sub, BaseMessage* msg, int size) {
	palTogglePad(LED_GPIO, LED4);
	_cnt++;

	return true;
}

