
#include <r2p/MessagePtrQueue.hpp>

namespace r2p {


MessagePtrQueue::MessagePtrQueue(BaseMessage *arrayp[], size_t length)
:
  ArrayQueue<BaseMessage *>(arrayp, length)
{}


} // namespace r2p
