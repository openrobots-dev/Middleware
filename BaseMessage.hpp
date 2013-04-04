#ifndef __BASEMESSAGE_HPP__
#define __BASEMESSAGE_HPP__

#include "ch.hpp"

class BaseMessage {
protected:
	uint16_t _ref_count;
public:
	BaseMessage(void);
	void reference(void);
	bool_t dereference(void);
	void reset(void);
}__attribute__((packed));

#endif
