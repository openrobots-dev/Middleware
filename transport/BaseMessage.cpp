#include "BaseMessage.hpp"

BaseMessage::BaseMessage(void) {
	_ref_count = 0;
}

/* called from within system lock */
void BaseMessage::reference(void) {
	_ref_count++;
}

/* called from within system lock */
bool_t BaseMessage::dereference(void) {
	return (--_ref_count == 0) ? true : false;
}

void BaseMessage::reset(void) {
	_ref_count = 0;
}
