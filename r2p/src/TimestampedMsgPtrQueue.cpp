
#include <r2p/TimestampedMsgPtrQueue.hpp>

namespace r2p {


TimestampedMsgPtrQueue::TimestampedMsgPtrQueue(Entry array[], size_t length)
:
  impl(array, length)
{}


} // namespace r2p
