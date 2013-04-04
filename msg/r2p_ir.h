#ifndef __R2P_IR_H_
#define TOPICS_H_

#include "BaseMessage.hpp"

#define IRRAW_ID		3001
#define IR1_ID			3011
#define IR2_ID			3012
#define IR3_ID			3013
#define IR4_ID			3014

struct IRRawSingle: public BaseMessage {
	uint16_t value;
}__attribute__((packed));

struct IRRaw: public BaseMessage {
	uint16_t value1;
	uint16_t value2;
	uint16_t value3;
	uint16_t value4;
}__attribute__((packed));

#endif /* __R2P_IR_H_ */
