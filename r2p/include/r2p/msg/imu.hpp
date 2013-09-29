#pragma once

namespace r2p {

struct TiltMsg: public Message {
	float angle;
} R2P_PACKED;

#define TILT_ID			(43 << 8)

}
