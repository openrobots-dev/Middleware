#pragma once

#include <r2p/common.hpp>
#include <ch.h>
#include <hal.h>

namespace r2p {

struct ledpub_conf {
	const char * topic;
	uint8_t led;
};

struct ledsub_conf {
	const char * topic;
};

ioportid_t led2gpio(unsigned led_id);
unsigned led2pin(unsigned led_id);

msg_t ledpub_node(void *arg);
msg_t ledsub_node(void *arg);

} /* namespace r2p */
