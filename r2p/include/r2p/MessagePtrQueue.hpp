
#ifndef __R2P__MESSAGEQUEUE_HPP__
#define __R2P__MESSAGEQUEUE_HPP__

#include <r2p/common.hpp>
#include <r2p/ArrayQueue.hpp>

namespace r2p {

class BaseMessage;


class MessagePtrQueue : public ArrayQueue<BaseMessage *> {
public:
  MessagePtrQueue(BaseMessage *arrayp[], size_t length);
};


} // namespace r2p
#endif // __R2P__MESSAGEQUEUE_HPP__
