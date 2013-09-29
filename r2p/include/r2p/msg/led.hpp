#pragma once



namespace r2p {

struct LedMsg: public Message {
#if R2P_LEDDEBUG
	uint32_t led;
	uint32_t value;
	uint32_t cnt;
#else
	uint32_t led; // FIXME: messages with size smaller than sizeof(void*) fail
	uint8_t value;
#endif
} R2P_PACKED;

} /* namespace r2p */

#define LEDS_ID			(10 << 8)
