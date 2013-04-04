#ifndef __MESSAGEQUEUE_HPP__
#define __MESSAGEQUEUE_HPP__

#include "BaseMessage.hpp"


class MessageQueue {
protected:
	BaseMessage ** _buffer; /**< @brief Pointer to message queue buffer. */
	BaseMessage ** _top; /**< @brief Pointer to first address after the buffer. */
	BaseMessage ** _write; /**< @brief Write pointer. */
	BaseMessage ** _read; /**< @brief Read pointer. */
	uint32_t _cnt; /**< @brief Queue counter. */
public:
	MessageQueue(BaseMessage ** buffer, uint32_t size);
	uint32_t size(void);
	uint32_t sizeI(void);
	BaseMessage * get(void);
	BaseMessage * getI(void);
	bool_t put(BaseMessage * msg);
	bool_t putI(BaseMessage * msg);
};

#endif
