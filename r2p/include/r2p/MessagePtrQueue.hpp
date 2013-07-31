#pragma once

#include <r2p/common.hpp>
#include <r2p/ArrayQueue.hpp>

namespace r2p {

class Message;


class MessagePtrQueue : public ArrayQueue<Message *> {
public:
  MessagePtrQueue(Message *arrayp[], size_t length);
};


} // namespace r2p
