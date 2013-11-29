#ifndef __R2P_IR_H_
#define __R2P_IR_H_

#define IRRAW_ID		(30 << 8)
#define IR1_ID			(31 << 8)
#define IR2_ID			(32 << 8)
#define IR3_ID			(33 << 8)
#define IR4_ID			(34 << 8)

#include "BaseMessage.hpp"

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
