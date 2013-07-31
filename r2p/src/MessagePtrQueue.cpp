
#include <r2p/MessagePtrQueue.hpp>

namespace r2p {


MessagePtrQueue::MessagePtrQueue(Message *arrayp[], size_t length)
:
  ArrayQueue<Message *>(arrayp, length)
{}


} // namespace r2p
