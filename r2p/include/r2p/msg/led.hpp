#pragma once

namespace r2p {

struct LedMsg: public Message {
	uint32_t led; // FIXME: messages with size smaller than sizeof(void*) fail
	uint8_t value;
} R2P_PACKED;

} /* namespace r2p */
